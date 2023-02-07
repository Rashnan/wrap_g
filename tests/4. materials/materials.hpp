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

    constexpr const char *stats_loc = "./tests/4. materials/stats.csv";

#if WRAP_G_BACKGROUND_RESOURCE_LOAD
    ////
    // background resource fetching

    // call blocking functions such as files/img loading in seperate thread.

    auto load_vert_src = utils::read_file_async(vert_path);
    auto load_frag_src = utils::read_file_async(frag_path);
    auto load_light_frag = utils::read_file_async(light_frag_path);
#endif

    ////
    // startup code

    // logic

    glm::vec3 world_up {0.0f, 1.0f, 0.0f};

    glm::vec3 cam_pos {0.0f, 1.0f, 1.0f};
    glm::vec3 light_pos {2.0f, 2.0f, -2.0f};
    glm::vec3 cube_pos {2.0f, 0.0f, -2.0f};

    wrap_g::observer camera;

    wrap_g::object cube_obj;
    wrap_g::object light_obj;

    cube_obj._model = glm::translate(cube_obj._model, cube_pos);
    light_obj._model = glm::translate(cube_obj._model, light_pos);

    wrap_g::perspective_camera pers_cam(30.0f, win.width() / (float)win.height(), 0.1f, 100.0f);
    wrap_g::dynamic_camera dyn_cam(cam_pos, cube_pos, world_up);

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
    constexpr auto coral = utils::rgba(1.0f, 0.5f, 0.31f, 1.0f);
    constexpr auto white = utils::hex("#ffffff");

    // opengl rendering

    // contains vao data & a program
    // and a model matrix
    wrap_g::cube cube_gl(win);
    wrap_g::cube light_gl(win);

#if !WRAP_G_BACKGROUND_RESOURCE_LOAD
    // reads the file right now.
    std::string vert_src = utils::read_file_sync(vert_path);
    bool success = cube_gl._base_gl._prog.quick({
        {GL_VERTEX_SHADER, {vert_src}},
        {GL_FRAGMENT_SHADER, {utils::read_file_sync(frag_path)}}
    });
    success = success && light_gl._base_gl._prog.quick({
        {GL_VERTEX_SHADER, {vert_src}},
        {GL_FRAGMENT_SHADER, {utils::read_file_sync(light_frag_path)}}
    });
#else
    // if shader source still has not loaded, force main thread to wait.
    std::string vert_src = load_vert_src.get();
    bool success = cube_gl._base_gl._prog.quick<std::string>({
        {GL_VERTEX_SHADER, {vert_src}},
        {GL_FRAGMENT_SHADER, {load_frag_src.get()}}
    });
    success = success && light_gl._base_gl._prog.quick({
        {GL_VERTEX_SHADER, {vert_src}},
        {GL_FRAGMENT_SHADER, {load_light_frag.get()}}
    });
#endif

    if (!success)
        return;

    enum class CUBE_OBJ_UNIFORMS { PROJ, VIEW, MODEL, COL, LIGHT_COL };
    const auto test_uniforms = cube_gl._base_gl._prog.uniform_locations("proj", "view", "model", "col", "light_col");

    cube_gl._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)CUBE_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
    cube_gl._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)CUBE_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
    cube_gl._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)CUBE_OBJ_UNIFORMS::MODEL], glm::value_ptr(cube_obj._model));
    cube_gl._base_gl._prog.set_uniform_vec<4>(test_uniforms[(int)CUBE_OBJ_UNIFORMS::COL], glm::value_ptr(coral));
    cube_gl._base_gl._prog.set_uniform_vec<4>(test_uniforms[(int)CUBE_OBJ_UNIFORMS::LIGHT_COL], glm::value_ptr(white));

    enum class LIGHT_OBJ_UNIFORMS { PROJ, VIEW, MODEL, COL };
    const auto light_uniforms = light_gl._base_gl._prog.uniform_locations("proj", "view", "model", "col");

    light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
    light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
    light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::MODEL], glm::value_ptr(light_obj._model));
    light_gl._base_gl._prog.set_uniform_vec<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::COL], glm::value_ptr(white));

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

            cube_gl._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)CUBE_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
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

            cube_gl._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)CUBE_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
            light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::PROJ], glm::value_ptr(pers_cam.m_proj));
        }


        cube_gl._base_gl._prog.set_uniform_mat<4>(test_uniforms[(int)CUBE_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));
        light_gl._base_gl._prog.set_uniform_mat<4>(light_uniforms[(int)LIGHT_OBJ_UNIFORMS::VIEW], glm::value_ptr(dyn_cam.m_view));

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