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

    glm::vec3 world_up {0.0f, 1.0f, 0.0f};

    wrap_g::observer camera;

    wrap_g::perspective_camera pers_cam(30.0f, win.width() / (float)win.height(), 0.1f, 100.0f);
    wrap_g::dynamic_camera dyn_cam({0.0f, 0.0f, 3.0f}, {0.0f, 0.0f, 0.0f}, world_up);

    bool first_mouse = false;

    // float last_x = win.width() / 2.0f, last_y = win.height() / 2.0f;
    glm::vec2 last_cursor = glm::vec2{win.width(), win.height()} / 2.0f;

    // the various sensitivities
    // look sens is for camera rotation along with the mouse
    // movement sens is for camera movement within the world
    // zoom sens is for camera zoom or fov control
    float look_sens = 300.0, move_sens = 10.0, zoom_sens = 100.0;
    
    // hide the cursor
    glfwSetInputMode(win.win(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
    bool success = test._base_gl._prog.quick({
        {GL_VERTEX_SHADER, {utils::read_file_sync(vert_path)}},
        {GL_FRAGMENT_SHADER, {utils::read_file_sync(frag_path)}}
    });
#else
    // if shader source still has not loaded, force main thread to wait.
    bool success = test._base_gl._prog.quick<std::string>({
        {GL_VERTEX_SHADER, {load_vert_src.get()}},
        {GL_FRAGMENT_SHADER, {load_frag_src.get()}}
    });
#endif

    if (!success)
        return;

    enum class TEST_UNIFORMS { PROJ, VIEW, MODEL, COL };
    const auto test_uniforms = test._base_gl._prog.uniform_locations("proj", "view", "model", "col");

    test._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)TEST_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
    test._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)TEST_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
    test._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)TEST_UNIFORMS::MODEL], glm::value_ptr(test._base._model));
    test._base_gl._prog.set_uniform_vec<4>(test_uniforms[(int)TEST_UNIFORMS::COL], glm::value_ptr(yellow));

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
            }
            else
            {
                // calculate how far the mouse is from last position
                glm::vec2 cursor_offset = glm::vec2{cursor.first, cursor.second} - last_cursor;
                // y axis is flipped as y is measured from top to bottom
                // top of screen is y=0 when we get the cursor position
                cursor_offset *= glm::vec2{1, -1} * look_sens * dt;
                dyn_cam.rotate(cursor_offset, world_up);
            }

            // update last cursor position
            last_cursor = glm::vec2{cursor.first, cursor.second};
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
            test._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)TEST_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
        }

        // * Reset all variables to their original values
        // * This includes position and cursor position and tex mix
        if (win.get_key(GLFW_KEY_R) == GLFW_PRESS)
        {
            glfwSetCursorPos(win.win(), (double)win.width() / 2.0, (double)win.height() / 2.0);
            first_mouse = true;
            dyn_cam.reset(world_up);
            pers_cam.reset_fov();
            test._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)TEST_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
        }


        test._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)TEST_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));

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