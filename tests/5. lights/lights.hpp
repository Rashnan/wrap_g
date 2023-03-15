#ifndef WRAP_G_TESTS_LIGHTS
#define WRAP_G_TESTS_LIGHTS

#include <iostream>

#include "../../src/utils.hpp"
#include "../../src/wrap_g.hpp"
#include "../../src/wrap_g_exp.hpp"

namespace wrap_tests
{

void create_lights() noexcept
{
    // time each process
    // outside as dt per frame is calculated using this
    utils::timer watch;

#if WRAP_G_DEBUG
    watch.start();
#endif

    // initialize glfw and set opengl version and some stuff
    wrap_g::wrap_g graphics;

    if (!graphics.valid())
        return;

    // create a window / context.
    // width: 800
    // height: 600
    // title: "Triangle Test Window."
    // underlying function also checks to see if glad is valid
    auto win = graphics.create_window(800, 600, "Textured Rect Test Window.");

    // check if underlying GLFWwindow* is valid
    if (win.win() == nullptr)
        return;
    
    // set window resize function to adjust viewport automatically
    // internally forwards to glfwSetFramebufferSizeCallback
    win.set_framebuffer_size_callback([](GLFWwindow *, GLint w, GLint h){ glViewport(0, 0, w, h); });

    // ensure ESC can always exit the program
    // internally forwards to glfwSetKeyCallback
    win.set_key_callback([](GLFWwindow *win, int key, int, int action, int){
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(win, true);
        }
    });

    // *supposed to set buffer swap rate to sync with monitor.
    // *not sure if works
    win.set_buffer_swap_interval(0);

    // hide the cursor
    // glfwSetInputMode(win.win(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    win.set_input_mode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

#if WRAP_G_DEBUG
    std::cout << "[main] Debug: Standard stuff time elapsed: " << watch.stop() << " ms \n";
    watch.start();
#endif

    ////
    // Resource locations

    constexpr const char *vert_path = "./tests/5. lights/vert.glsl";
    constexpr const char *frag_path = "./tests/5. lights/frag.glsl";
    constexpr const char *light_frag_path = "./tests/5. lights/light_frag.glsl";

    constexpr const char *stats_loc = "./tests/5. lights/stats.csv";
    
#if WRAP_G_BACKGROUND_RESOURCE_LOAD
    ////
    // background resource fetching

    // call blocking functions such as files/img loading in seperate thread.

    auto load_vert_src = utils::read_file_async(vert_path);
    auto load_frag_src = utils::read_file_async(frag_path);
    auto load_light_frag_src = utils::read_file_async(light_frag_path);
#endif

    ////
    // startup code

    // logic

    glm::vec3 world_up {0.0f, 1.0f, 0.0f};

    glm::vec3 cam_start_pos {-1.0f, 0.0f, 1.0f};
    glm::vec3 light_pos {2.0f, 2.0f, -3.0f};
    glm::vec3 cube_pos {0.0f, 0.0f, -2.0f};

    // wrap_g math stuff

    wrap_g::observer camera;

    wrap_g::object cube_obj;
    wrap_g::object light_obj;

    cube_obj._model = glm::translate(cube_obj._model, cube_pos);
    light_obj._model = glm::translate(light_obj._model, light_pos);
    light_obj._model = glm::scale(light_obj._model, glm::vec3{0.25f});

    cube_obj._normal_mat = glm::mat3(glm::transpose(glm::inverse(cube_obj._model)));

    wrap_g::perspective_camera pers_cam(30.0f, win.width() / (float)win.height(), 0.1f, 100.0f);
    wrap_g::dynamic_camera dyn_cam(cam_start_pos, cube_pos, world_up);

    // first mouse needed for mouse rotation
    bool first_mouse = false;

    // float last_x = win.width() / 2.0f, last_y = win.height() / 2.0f;
    glm::vec2 last_cursor = glm::vec2{win.width(), win.height()} / 2.0f;

    // the various sensitivities
    // look sens is for camera rotation along with the mouse
    // movement sens is for camera movement within the world
    // zoom sens is for camera zoom or fov control
    float look_sens = 300.0, move_sens = 10.0, zoom_sens = 100.0;
    float cube_rotation_speed = 10.0f;

    // the struct containing info about the material
    // diffuse and specular contain ints to the location of the texture unit
    // storing their maps
    struct Material {
        int diffuse;
        int specular;
        float shininess;
    };

    Material cube_mat{
        .diffuse = 0,
        .specular = 1,
        .shininess = 64.0f
    };

    // ! note different from frag shader
    // ! no pos
    struct Light {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
    };

    Light light_mat {
        .ambient = glm::vec3{1.0f},
        .diffuse = glm::vec3{1.0f},
        .specular = glm::vec3{1.0f}
    };

    // gives a glm::vec4 containing the rgba color values
    constexpr auto blue = utils::hex("#111b24");
    constexpr auto white = utils::hex("#ffffff");

    // opengl rendering

    // contains vao data & a program
    // and a model matrix
    wrap_g::cube cube_gl(win);
    wrap_g::cube light_gl(win);

    // store the source code of the shaders
    std::string vert_src, frag_src, light_frag_src;

#if !WRAP_G_BACKGROUND_RESOURCE_LOAD
    // reads the file right now.
    vert_src = utils::read_file_sync(vert_path);
    frag_src = utils::read_file_sync(frag_path);
    light_frag_src = utils::read_file_sync(light_frag_path);
    
    bool success = cube_gl._base_gl._prog.quick({
        {GL_VERTEX_SHADER, {vert_src}},
        {GL_FRAGMENT_SHADER, {frag_src}}
    });
    success = success && light_gl._base_gl._prog.quick({
        {GL_VERTEX_SHADER, {vert_src}},
        {GL_FRAGMENT_SHADER, {light_frag_src}}
    });
#else
    // if shader source still has not loaded, force main thread to wait.
    vert_src = load_vert_src.get();
    frag_src = load_frag_src.get();
    light_frag_src = load_light_frag_src.get();

    bool success = cube_gl._base_gl._prog.quick({
        {GL_VERTEX_SHADER, {vert_src}},
        {GL_FRAGMENT_SHADER, {frag_src}}
    });
    success = success && light_gl._base_gl._prog.quick({
        {GL_VERTEX_SHADER, {vert_src}},
        {GL_FRAGMENT_SHADER, {light_frag_src}}
    });
#endif

    if (!success)
        return;
        
    // index of specific uniform location in the vector for cube obj
    enum class CUBE_OBJ_UNIFORMS {
        PROJ, VIEW, MODEL, NORMAL_MAT,
        MAT_DIFFUSE, MAT_SPECULAR, MAT_SHININESS,
        LIGHT_POSITION, LIGHT_AMBIENT, LIGHT_DIFFUSE, LIGHT_SPECULAR,
        CAM_POS
    };

    // store the uniform locations in a vector for cube obj
    auto cube_uniforms = cube_gl._base_gl._prog.uniform_locations(
        "proj", "view", "model", "normal_mat",
        "material.diffuse", "material.specular", "material.shininess",
        "light.position", "light.ambient", "light.diffuse", "light.specular",
        "cam_pos"
    );

    // setting said uniforms

    cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
    cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
    cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MODEL], glm::value_ptr(cube_obj._model));
    cube_gl._base_gl._prog.set_uniform_mat<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::NORMAL_MAT], glm::value_ptr(cube_obj._normal_mat));

    // diffuse and specular hold ints to store the texture unit of their respective maps
    cube_gl._base_gl._prog.set_uniform(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_DIFFUSE], cube_mat.diffuse);
    cube_gl._base_gl._prog.set_uniform(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_SPECULAR], cube_mat.specular);
    cube_gl._base_gl._prog.set_uniform(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_SHININESS], cube_mat.shininess);

    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_POSITION], glm::value_ptr(light_pos));
    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_AMBIENT], glm::value_ptr(light_mat.ambient));
    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_DIFFUSE], glm::value_ptr(light_mat.diffuse));
    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_SPECULAR], glm::value_ptr(light_mat.specular));

    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::CAM_POS], glm::value_ptr(dyn_cam.m_pos));

    // index of specific uniform location in the vector for light obj
    enum class LIGHT_OBJ_UNIFORMS { PROJ, VIEW, MODEL, COL };
    
    // store the uniform locations in a vector for light obj
    auto light_uniforms = light_gl._base_gl._prog.uniform_locations("proj", "view", "model", "col");

    // setting said uniforms

    light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
    light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
    light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::MODEL], glm::value_ptr(light_obj._model));
    light_gl._base_gl._prog.set_uniform_vec<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::COL], glm::value_ptr(white));

    // check if shaders are being reloaded

    bool reloading_shaders = false;

#if WRAP_G_BACKGROUND_RESOURCE_LOAD
    bool loaded_mat_list = false;
    bool loaded_vert_src = false;
    bool loaded_frag_src = false;
    bool loaded_light_frag_src = false;
#endif

    glEnable(GL_DEPTH_TEST);

#if WRAP_G_DEBUG
    std::cout << "[main] Debug: Starting code time elapsed: " << watch.stop() << " ms \n";
#endif

#if WRAP_G_DEBUG
    utils::metrics tracker;
    tracker.start_tracking();
#endif
    float dt = 0.01;
}

}

#endif