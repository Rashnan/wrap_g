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

    constexpr const char *stats_loc = "./tests/5. lights/stats.csv";
    
#if WRAP_G_BACKGROUND_RESOURCE_LOAD
    ////
    // background resource fetching

    // call blocking functions such as files/img loading in seperate thread.

    auto load_vert_src = utils::read_file_async(vert_path);
    auto load_frag_src = utils::read_file_async(frag_path);
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
    
}

}

#endif