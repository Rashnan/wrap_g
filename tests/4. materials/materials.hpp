#ifndef WRAP_G_TESTS_MATERIALS
#define WRAP_G_TESTS_MATERIALS

#include <iostream>

#include "../../src/utils.hpp"
#include "../../src/wrap_g.hpp"
#include "../../src/wrap_g_exp.hpp"

namespace wrap_tests
{
    
void create_materials() noexcept
{
#if WRAP_G_DEBUG
    // time each process
    utils::timer watch;
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

#if WRAP_G_DEBUG
    std::cout << "[main] Debug: Standard stuff time elapsed: " << watch.stop() << " ms \n";
    watch.start();
#endif
    
    ////
    // Resource locations

    constexpr const char *vert_path = "./tests/4. materials/vert.glsl";
    constexpr const char *frag_path = "./tests/4. materials/frag.glsl";
    constexpr const char *light_frag_path = "./tests/4. materials/light_frag.glsl";

    constexpr const char *materials_list_path = "./tests/4. materials/materials list.csv";

    constexpr const char *stats_loc = "./tests/4. materials/stats.csv";

#if WRAP_G_BACKGROUND_RESOURCE_LOAD
    ////
    // background resource fetching

    // call blocking functions such as files/img loading in seperate thread.

    auto load_vert_src = utils::read_file_async(vert_path);
    auto load_frag_src = utils::read_file_async(frag_path);
    auto load_light_frag_src = utils::read_file_async(light_frag_path);
    auto load_mat_list = utils::read_file_async(materials_list_path);
#endif

    ////
    // startup code

    // logic

    glm::vec3 world_up {0.0f, 1.0f, 0.0f};

    glm::vec3 cam_start_pos {-1.0f, 0.0f, 1.0f};
    glm::vec3 light_pos {2.0f, 2.0f, -3.0f};
    glm::vec3 cube_pos {0.0f, 0.0f, -2.0f};

    wrap_g::observer camera;

    wrap_g::object cube_obj;
    wrap_g::object light_obj;

    cube_obj._model = glm::translate(cube_obj._model, cube_pos);
    light_obj._model = glm::translate(light_obj._model, light_pos);
    light_obj._model = glm::scale(light_obj._model, glm::vec3{0.25f});

    cube_obj._normal_mat = glm::mat3(glm::transpose(glm::inverse(cube_obj._model)));

    wrap_g::perspective_camera pers_cam(30.0f, win.width() / (float)win.height(), 0.1f, 100.0f);
    wrap_g::dynamic_camera dyn_cam(cam_start_pos, cube_pos, world_up);

    bool first_mouse = false;

    // float last_x = win.width() / 2.0f, last_y = win.height() / 2.0f;
    glm::vec2 last_cursor = glm::vec2{win.width(), win.height()} / 2.0f;

    // the various sensitivities
    // look sens is for camera rotation along with the mouse
    // movement sens is for camera movement within the world
    // zoom sens is for camera zoom or fov control
    float look_sens = 300.0, move_sens = 10.0, zoom_sens = 100.0;
    float cube_rotation_speed = 10.0f;

    struct Material {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;
    };

    std::string mat_list_str;

#if !WRAP_G_BACKGROUND_RESOURCE_LOAD
    mat_list_str = utils::read_file_sync(materials_list_path);    
#else
    mat_list_str = load_mat_list.get();
#endif

    // TODO: create csv reader and allow switching between materials using keypress
    std::vector<Material> materials_list;

    Material cube_mat {
        .ambient = glm::vec3{0.1745f, 0.01175f, 0.01175f},
        .diffuse = glm::vec3{0.61424f, 0.04136f, 0.04136f},
        .specular = glm::vec3{0.727811f, 0.626959f, 0.626959f},
        .shininess = 128.0f * 0.6f,
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
    
    // hide the cursor
    glfwSetInputMode(win.win(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // gives a glm::vec4 containing the rgba color values
    constexpr auto blue = utils::hex("#111b24");
    constexpr auto yellow = utils::hex("#d2cb7f");
    constexpr auto coral = utils::rgba(1.0f, 0.5f, 0.31f, 1.0f);
    constexpr auto white = utils::hex("#ffffff");

    // opengl rendering

    // contains vao data & a program
    // and a model matrix
    wrap_g::cube cube_gl(win);
    wrap_g::cube light_gl(win);

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

    enum class CUBE_OBJ_UNIFORMS {
        PROJ, VIEW, MODEL, NORMAL_MAT,
        MAT_AMBIENT, MAT_DIFFUSE, MAT_SPECULAR, MAT_SHININESS,
        LIGHT_POSITION, LIGHT_AMBIENT, LIGHT_DIFFUSE, LIGHT_SPECULAR,
        CAM_POS
    };

    auto cube_uniforms = cube_gl._base_gl._prog.uniform_locations(
        "proj", "view", "model", "normal_mat",
        "material.ambient", "material.diffuse", "material.specular", "material.shininess",
        "light.position", "light.ambient", "light.diffuse", "light.specular",
        "cam_pos"
    );

    cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
    cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
    cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MODEL], glm::value_ptr(cube_obj._model));
    cube_gl._base_gl._prog.set_uniform_mat<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::NORMAL_MAT], glm::value_ptr(cube_obj._normal_mat));

    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_AMBIENT], glm::value_ptr(cube_mat.ambient));
    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_DIFFUSE], glm::value_ptr(cube_mat.diffuse));
    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_SPECULAR], glm::value_ptr(cube_mat.specular));
    cube_gl._base_gl._prog.set_uniform(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_SHININESS], cube_mat.shininess);

    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_POSITION], glm::value_ptr(light_pos));
    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_AMBIENT], glm::value_ptr(light_mat.ambient));
    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_DIFFUSE], glm::value_ptr(light_mat.diffuse));
    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_SPECULAR], glm::value_ptr(light_mat.specular));

    cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::CAM_POS], glm::value_ptr(dyn_cam.m_pos));

    enum class LIGHT_OBJ_UNIFORMS { PROJ, VIEW, MODEL, COL };
    auto light_uniforms = light_gl._base_gl._prog.uniform_locations("proj", "view", "model", "col");

    light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
    light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
    light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::MODEL], glm::value_ptr(light_obj._model));
    light_gl._base_gl._prog.set_uniform_vec<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::COL], glm::value_ptr(white));

    bool reloading_shaders = false;
    bool loaded_vert_src = false;
    bool loaded_frag_src = false;
    bool loaded_light_frag_src = false;

    glEnable(GL_DEPTH_TEST);

#if WRAP_G_DEBUG
    std::cout << "[main] Debug: Starting code time elapsed: " << watch.stop() << " ms \n";
#endif

#if WRAP_G_DEBUG
    utils::metrics tracker;
    tracker.start_tracking();
#endif
    float dt = 0.01;

    while (!win.get_should_close())
    {
        ////
        // event handling
        
        // get events such as mouse input
        // checks every time for event
        glfwPollEvents();

        if (reloading_shaders)
        {
#if !WRAP_G_BACKGROUND_RESOURCE_LOAD
            // reads the file right now.
            vert_src = utils::read_file_sync(vert_path);
            frag_src = utils::read_file_sync(frag_path);
            light_frag_src = utils::read_file_sync(light_frag_path);
            
            cube_gl._base_gl._prog.flush_shaders();
            success = cube_gl._base_gl._prog.quick({
                {GL_VERTEX_SHADER, {vert_src}},
                {GL_FRAGMENT_SHADER, {frag_src}}
            });

            light_gl._base_gl._prog.flush_shaders();
            success = success && light_gl._base_gl._prog.quick({
                {GL_VERTEX_SHADER, {vert_src}},
                {GL_FRAGMENT_SHADER, {light_frag_src}}
            });
#else
            // if shader source still has not loaded, force main thread to wait.

            load_vert_src = std::async([&loaded_vert_src, &vert_path, &vert_src](){
                loaded_vert_src = false;
                vert_src = utils::read_file_sync(vert_path);
                loaded_vert_src = true;
                return vert_src;
            });
            
            load_frag_src = std::async([&loaded_frag_src, &frag_path, &frag_src](){
                loaded_frag_src = false;
                frag_src = utils::read_file_sync(frag_path);
                loaded_frag_src = true;
                return frag_src;
            });
            
            load_light_frag_src = std::async([&loaded_light_frag_src, &light_frag_path, &light_frag_src](){
                loaded_light_frag_src = false;
                light_frag_src = utils::read_file_sync(light_frag_path);
                loaded_light_frag_src = true;
                return light_frag_src;
            });

            if (loaded_vert_src && loaded_frag_src && loaded_light_frag_src)
            {
                cube_gl._base_gl._prog.flush_shaders();
                success = cube_gl._base_gl._prog.quick({
                    {GL_VERTEX_SHADER, {vert_src}},
                    {GL_FRAGMENT_SHADER, {frag_src}}
                });

                light_gl._base_gl._prog.flush_shaders();
                success = success && light_gl._base_gl._prog.quick({
                    {GL_VERTEX_SHADER, {vert_src}},
                    {GL_FRAGMENT_SHADER, {light_frag_src}}
                });
            }
#endif

            // Increases in uniforms cannot be hot reloaded as the new variables need to be initialized in the cpp file
            // Depending on changes GPU mmay optimize out some uniforms but this will NOT cause any errors in setting them

            cube_uniforms = cube_gl._base_gl._prog.uniform_locations(
                "proj", "view", "model", "normal_mat",
                "material.ambient", "material.diffuse", "material.specular", "material.shininess",
                "light.position", "light.ambient", "light.diffuse", "light.specular",
                "cam_pos"
            );

            cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
            cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
            cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MODEL], glm::value_ptr(cube_obj._model));
            cube_gl._base_gl._prog.set_uniform_mat<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::NORMAL_MAT], glm::value_ptr(cube_obj._normal_mat));

            cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_AMBIENT], glm::value_ptr(cube_mat.ambient));
            cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_DIFFUSE], glm::value_ptr(cube_mat.diffuse));
            cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_SPECULAR], glm::value_ptr(cube_mat.specular));
            cube_gl._base_gl._prog.set_uniform(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MAT_SHININESS], cube_mat.shininess);

            cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_POSITION], glm::value_ptr(light_pos));
            cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_AMBIENT], glm::value_ptr(light_mat.ambient));
            cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_DIFFUSE], glm::value_ptr(light_mat.diffuse));
            cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_SPECULAR], glm::value_ptr(light_mat.specular));

            cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::CAM_POS], glm::value_ptr(dyn_cam.m_pos));

            light_uniforms = light_gl._base_gl._prog.uniform_locations("proj", "view", "model", "col");

            light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
            light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
            light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::MODEL], glm::value_ptr(light_obj._model));
            light_gl._base_gl._prog.set_uniform_vec<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::COL], glm::value_ptr(white));

            reloading_shaders = false;
        }

        // look direction
        // rotate camera along with mouse rotation
        // but only if user is pressing left click
        if (win.get_mouse_button(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            auto cursor = win.get_cursor_position();
            
            if (first_mouse)
            {
                // basically ignore first mouse
                first_mouse = false;
                // update last cursor position
                last_cursor = glm::vec2{cursor.first, cursor.second};
            }
            else
            {
                // calculate how far the mouse is from last position
                glm::vec2 cursor_offset = glm::vec2{cursor.first, cursor.second} - last_cursor;
                // update last cursor position
                last_cursor = glm::vec2{cursor.first, cursor.second};
                // y axis is flipped as y is measured from top to bottom
                // top of screen is y=0 when we get the cursor position
                cursor_offset *= glm::vec2{1, -1} * look_sens * dt;
                dyn_cam.rotate(cursor_offset, world_up);
            }
        }

        // movement

        // move left
        if (win.get_key(GLFW_KEY_A) == GLFW_PRESS)
        {
            auto&& offset = - dyn_cam.m_right * move_sens * dt;
            dyn_cam.move(offset);
        }

        // move right
        if (win.get_key(GLFW_KEY_D) == GLFW_PRESS)
        {
            auto&& offset = dyn_cam.m_right * move_sens * dt;
            dyn_cam.move(offset);
        }
        
        // move forward
        if (win.get_key(GLFW_KEY_W) == GLFW_PRESS)
        {
            auto&& offset = dyn_cam.m_front * move_sens * dt;
            dyn_cam.move(offset);
        }

        // move backward
        if (win.get_key(GLFW_KEY_S) == GLFW_PRESS)
        {
            auto&& offset = - dyn_cam.m_front * move_sens * dt;
            dyn_cam.move(offset);
        }
        
        // move upwards
        if (win.get_key(GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            auto&& offset = dyn_cam.m_up * move_sens * dt;
            dyn_cam.move(offset);
        }

        // move downwards
        if (win.get_key(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            auto&& offset = - dyn_cam.m_up * move_sens * dt;
            dyn_cam.move(offset);
        }
        
        // zoom event
        if (win.get_key(GLFW_KEY_Z) == GLFW_PRESS)
        {
            pers_cam.adjust_fov(-(win.get_key(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ? -1.0 : 1.0) * zoom_sens * dt);

            cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
            light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
        }

        // * Reset all variables to their original values
        // * This includes position and cursor position and tex mix
        if (win.get_key(GLFW_KEY_R) == GLFW_PRESS)
        {
            glfwSetCursorPos(win.win(), (double)win.width() / 2.0, (double)win.height() / 2.0);
            first_mouse = true;
            dyn_cam.reset(world_up);
            pers_cam.reset_fov();

            cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
            light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
        }

        if (win.get_key(GLFW_KEY_T) == GLFW_PRESS && !reloading_shaders){ reloading_shaders = true; }

        cube_gl._base_gl._prog.set_uniform_vec<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::CAM_POS], glm::value_ptr(dyn_cam.m_pos));
        cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
        light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));

        cube_obj._model = glm::rotate(cube_obj._model, glm::radians(45.0f) * cube_rotation_speed * dt, glm::vec3{1, 2, 3});
        cube_obj._normal_mat = glm::mat3(glm::transpose(glm::inverse(cube_obj._model)));

        cube_gl._base_gl._prog.set_uniform_mat<4>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::MODEL], glm::value_ptr(cube_obj._model));
        cube_gl._base_gl._prog.set_uniform_mat<3>(cube_uniforms[(int)CUBE_OBJ_UNIFORMS::NORMAL_MAT], glm::value_ptr(cube_obj._normal_mat));
        
        watch.start();

        ////
        // rendering

        // set the color that will be used when glClear is called on the color buffer bit
        glClearColor(blue.r, blue.g, blue.b, blue.a);

        // use this to reset the color and reset the depth buffer bit
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        cube_gl.render();
        light_gl.render();

        // swap the buffers to show the newly drawn frame
        win.swap_buffers();

        dt = watch.stop();
#if WRAP_G_DEBUG
        tracker.track_frame(dt);
#endif
        dt = glm::clamp(dt, 0.0001f, 0.01f);
    }
#if WRAP_G_DEBUG
    tracker.finish_tracking();
    tracker.save(stats_loc);
#endif
}

} // namespace wrap_tests

#endif