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

    constexpr const char *stats_loc = "./tests/4. materials/stats.csv";

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

    // consists of glm::mat4 one for proj and one for view
    wrap_g::observer<> camera;

    camera._proj = glm::perspective(30.0f, win.width()/(float)win.height(), 0.1f, 100.0f);

    float look_sens = 30.0f, move_sens = 10.0f, zoom_sens = 100.0f;

    // gives a glm::vec4 containing the rgba color values
    constexpr auto blue = utils::hex("#111b24");
    constexpr auto yellow = utils::hex("#d2cb7f");

    // opengl rendering

    // contains vao data & a program
    // and a model matrix
    // wrap_g::rect test(win);
    wrap_g::cube test(win);

#if !WRAP_G_BACKGROUND_RESOURCE_LOAD
    // reads the file right now.
    bool success = test.prog_quick({
        {GL_VERTEX_SHADER, {utils::read_file_sync(vert_path)}},
        {GL_FRAGMENT_SHADER, {utils::read_file_sync(frag_path)}}
    });
#else
    // if shader source still has not loaded, force main thread to wait.
    bool success = test.prog_quick<std::string>({
        {GL_VERTEX_SHADER, {load_vert_src.get()}},
        {GL_FRAGMENT_SHADER, {load_frag_src.get()}}
    });
#endif

    if (!success)
        return;

    test.save_uniforms("proj", "view", "model", "col");
    
    test.set_uniform_mat<4>("proj", glm::value_ptr(camera._proj));
    test.set_uniform_mat<4>("view", glm::value_ptr(camera._view));
    test.set_uniform_mat<4>("model", glm::value_ptr(test._base._model));
    test.set_uniform_vec<4>("col", glm::value_ptr(yellow));

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

        if (win.get_key(GLFW_KEY_W) == GLFW_PRESS)
        {
            camera._view = glm::translate(camera._view, utils::front(camera._view) * dt * move_sens);
        }

        if (win.get_key(GLFW_KEY_S) == GLFW_PRESS)
        {
            camera._view = glm::translate(camera._view, -utils::front(camera._view) * dt * move_sens);
        }
        
        if (win.get_key(GLFW_KEY_A) == GLFW_PRESS)
        {
            camera._view = glm::translate(camera._view, -utils::right(camera._view) * dt * move_sens);
        }

        if (win.get_key(GLFW_KEY_D) == GLFW_PRESS)
        {
            camera._view = glm::translate(camera._view, utils::right(camera._view) * dt * move_sens);
        }

        if (win.get_key(GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            camera._view = glm::translate(camera._view, utils::up(camera._view) * dt * move_sens);
        }

        if (win.get_key(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            camera._view = glm::translate(camera._view, -utils::up(camera._view) * dt * move_sens);
        }
        
        test.set_uniform_mat<4>("view", glm::value_ptr(camera._view));

        watch.start();

        ////
        // rendering

        // set the color that will be used when glClear is called on the color buffer bit
        glClearColor(blue.r, blue.g, blue.b, blue.a);

        // use this to reset the color and reset the depth buffer bit
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        test.render();

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