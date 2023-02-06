#ifndef WRAP_G_TESTS_MOVING_AROUND_CUBES
#define WRAP_G_TESTS_MOVING_AROUND_CUBES

#include <iostream>

#include "../../src/utils.hpp"
#include "../../src/wrap_g.hpp"

namespace wrap_tests
{

void create_moving_around_cubes() noexcept
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

    constexpr const char *img_path_1 = "./tests/res/images/wall.jpg";
    constexpr const char *img_path_2 = "./tests/res/images/awesomeface.png";

    utils::stb_image img_loader_1;
    utils::stb_image img_loader_2;

    constexpr const char *vert_path = "./tests/3. moving around cubes/vert.glsl";
    constexpr const char *frag_path = "./tests/3. moving around cubes/frag.glsl";

    constexpr const char *stats_loc = "./tests/3. moving around cubes/stats.csv";

#if WRAP_G_BACKGROUND_RESOURCE_LOAD
    ////
    // background resource fetching

    // call blocking functions such as files/img loading in seperate thread.

    auto load_img_1 = img_loader_1.load_file_async(img_path_1);
    auto load_img_2 = img_loader_2.load_file_async(img_path_2, true);

    auto load_vert_src = utils::read_file_async(vert_path);
    auto load_frag_src = utils::read_file_async(frag_path);
#endif

    ////
    // startup code

    // logic

    // positions of all the cubes
    auto cube_positions = std::array{
        glm::vec3( 0.0f,  0.0f,   0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f,  -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f,  -3.5f),
        glm::vec3(-1.7f,  3.0f,  -7.5f),
        glm::vec3( 1.3f, -2.0f,  -2.5f),
        glm::vec3( 1.5f,  2.0f,  -2.5f),
        glm::vec3( 1.5f,  0.2f,  -1.5f),
        glm::vec3(-1.3f,  1.0f,  -1.5f),
    };

    // the tex mix determines how much the first/second texture will be visible
    // if tex mix is 1.0 then second texture is visible
    // if tex mix is 0.0 then first texture is visible
    // if tex mix is greater than 1.0 then a negative of the first texture will be seen along with the second texture
    // if tex mix is lesser than 0.0 then a negative of the second texture will be seen along with the first texture
    // this behaviour can be removed by clamping the tex mix val
    // the starting value of the tex mix
    // the tex mix sens determines how much the tex mix will change with user input
    float starting_tex_mix = 0.5f, tex_mix = starting_tex_mix, tex_mix_sens = 1.0f;

    // gives a glm::vec4 containing the rgba color values
    constexpr auto blue = utils::hex("#111b24");

    // the starting position of the camera
    glm::vec3 camera_start_pos = {0.0f, 0.0f, 1.0f};
    // the current psition of the camera
    glm::vec3 camera_pos = camera_start_pos;
    // the position where the camera is focused onto at the start
    glm::vec3 camera_start_looking_at = {0.0f, 0.0f, 0.0f};

    // the world up vector
    glm::vec3 world_up = glm::vec3{0.0f, 1.0f, 0.0f};
    
    // the three perpendicular axises of the camera: front (dir), right, and up
    // calculated using right hand rule and cross product
    glm::vec3 camera_dir = glm::normalize(camera_start_looking_at - camera_start_pos);
    glm::vec3 camera_right = glm::normalize(glm::cross(camera_dir, world_up));
    glm::vec3 camera_up = glm::normalize(glm::cross(camera_right, camera_dir));
    
    // the pitch and yaw of the camera direction
    // first mouse determines whether last_x and last_y where set or not
    // this will be used to calculate camera rotation along with the mouse
    float camera_pitch = glm::degrees(glm::asin(camera_dir.y));
    float camera_yaw = glm::degrees(glm::asin(camera_dir.z / glm::cos(glm::radians(camera_pitch))));
    bool first_mouse = false;

    // float last_x = win.width() / 2.0f, last_y = win.height() / 2.0f;
    glm::vec2 last_cursor = glm::vec2{win.width(), win.height()} / 2.0f;

    // the fov of the camera at the start
    float starting_fov = 30.0f;
    // the current fov
    float fov = starting_fov;
    // the various sensitivities
    // look sens is for camera rotation along with the mouse
    // movement sens is for camera movement within the world
    // zoom sens is for camera zoom or fov control
    float look_sens = 300.0, movement_sens = 10.0, zoom_sens = 100.0;

    // the camera projection and view matrix
    // the model matrix for a single cube. (all cubes will use this matrix just modified later)
    glm::mat4 proj{1.0f}, view{1.0f}, model{1.0f};
    
    // this would set the projection matrix to be orthographic. *NOTE: zoom would need to be handled differently.
    // Leftmost visible point is -2 points from camera, then right most (2), then bottom most (-2) and top most (2)
    // Final two parameters 0.1f is the closest and 10.0f is the furthest visible objects from infront of the camera
    // proj = glm::ortho<float>(-2, 2, -2, 2, 0.1f, 10.0f);
    // set a perspective projection with the nearest visible objects being 0.1f infront of the camera
    // and the furthest being 10.0f from the camera
    proj = glm::perspective(glm::radians(fov), (float)win.width()/win.height(), 0.1f, 100.0f);
    view = glm::lookAt(camera_start_pos, camera_start_looking_at, glm::vec3{0.0f, 1.0f, 0.0f});

    // hide the cursor
    glfwSetInputMode(win.win(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // opengl render code

    // create a vao to store vertices and a shader program that defines how the vertices appear on the window
    auto vao = win.create_vao();
    auto prog = win.create_program();
    auto tex1 = win.create_texture(GL_TEXTURE_2D);
    auto tex2 = win.create_texture(GL_TEXTURE_2D);

    // creates a 3d cube in compile time
    // opengl negative z is towards screen
    
    constexpr auto verts = utils::gen_cube_verts(glm::vec3{-0.5f}, glm::vec3{0.5f});
    
    // creates a 2d coord within the texture map for each vertex

    constexpr auto tex_coords = utils::gen_cube_texcoords_single_face();

    // define_attrib defines an attribute in the shader and enables it
    // (**) inside brackets is paramter number
    // defines a 3 (third) float (fourth) attribute to be in attribute position 0 (second)
    // and the buffer to get this attribute from is the buffer at binding position 0 (first)
    // and at an offset 0 (default sixth) from the start of the buffer pointer and is not
    // normalized (default fifth)
    // this parameter is the position of each vertex
    vao.define_attrib(0, 0, 3, GL_FLOAT);
    // this paremter is the texture coordinate for each vertex
    vao.define_attrib(1, 1, 2, GL_FLOAT);
    
    // creates an array buffer at binding position 0 (first) of size verts.size() * sizeof(glm::vec3) (second)
    // and the pointer to it is verts.data() (third) and the flag enabled is GL_MAP_READ_BIT (fourth)
    // offset from pointer is 0 (default fifth)
    // underlying:
    // - creates array buffer
    // - creates fixed size storage (opengl 4.5+)
    // - creates variable size storage (opengl 3.3+ & 4.3+)
    // - binds the buffer to the vertex array object
    // - stores id of buffer to delete at destructor
    // * template parameter is used to tell the stride.. distance between vertices in memory
    // * as this is often known at compile time.
    // * stride can be manually set using a GLsizei argument between the data pointer and the flag
    // ! be careful as this may cause headaches if set incorrectly
    // creates the buffer for the vertex positions
    vao.create_array_buffer<const glm::vec3>(0, verts.size() * sizeof(glm::vec3), verts.data(), GL_MAP_READ_BIT);
    
    // creates the buffer for the texture coordinates
    vao.create_array_buffer<const glm::vec2>(1, tex_coords.size() * sizeof(glm::vec2), tex_coords.data(), GL_MAP_READ_BIT);

    // GL_TEXTURE_WRAP indicates what would happen if texture coords go outside of range (0.0, 0.0) and (1.0, 1.0)
    // rst -> xyz
    // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
    // GL_TEXTURE_MIN_FILTER defines the minifying function used if lower detail samples are needed.
    // GL_TEXTURE_MAG_FILTER defines the magnifying function used if hgiher detail samples are needed.
    // bind unit indicates the texture unit the current texture should be bound to.

    tex1.set_param(GL_TEXTURE_WRAP_S, GL_REPEAT);
    tex1.set_param(GL_TEXTURE_WRAP_T, GL_REPEAT);
    tex1.set_param(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    tex1.set_param(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    tex1.bind_unit(0);

    tex2.set_param(GL_TEXTURE_WRAP_S, GL_REPEAT);
    tex2.set_param(GL_TEXTURE_WRAP_T, GL_REPEAT);
    tex2.set_param(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    tex2.set_param(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    tex2.bind_unit(1);
    
    // quick method to compile and link provided shader files
    // multiple vertex and fragment shaders can be provided and other types of shaders as well
    // template argument tells whether all provided const char* (the vert_src and frag_src) is 
    // a string containing the shader source or a relative path to the file containing the src
    // if true then uses utils to open and read file
    // if false uses string as glsl code directly
    // both branches do essentially the same thing
#if !WRAP_G_BACKGROUND_RESOURCE_LOAD
    // reads the file right now.
    bool success = prog.quick({
        {GL_VERTEX_SHADER, {utils::read_file_sync(vert_path)}},
        {GL_FRAGMENT_SHADER, {utils::read_file_sync(frag_path)}}
    });
#else
    // if shader source still has not loaded, force main thread to wait.
    bool success = prog.quick({
        {GL_VERTEX_SHADER, {load_vert_src.get()}},
        {GL_FRAGMENT_SHADER, {load_frag_src.get()}}
    });
#endif
    if (!success)
        return;
        
    // ensure sampler2D uniform is provided as an int and is the same as 
    // the unit the texture is bound to.
    prog.set_uniform<int>(prog.uniform_location("tex1"), 0);

    // ensure sampler2D uniform is provided as an int and is the same as 
    // the unit the texture is bound to.
    prog.set_uniform<int>(prog.uniform_location("tex2"), 1);

    int tex_mix_loc = prog.uniform_location("tex_mix");
    prog.set_uniform<float>(tex_mix_loc, tex_mix);
    
    int proj_loc = prog.uniform_location("proj"),
        view_loc = prog.uniform_location("view"),
        model_loc = prog.uniform_location("model");

    prog.set_uniform_mat<4>(proj_loc, glm::value_ptr(proj));
    prog.set_uniform_mat<4>(view_loc, glm::value_ptr(view));
    prog.set_uniform_mat<4>(model_loc, glm::value_ptr(model));
    
#if WRAP_G_BACKGROUND_RESOURCE_LOAD
    ////
    // Resource Fetching Threads Done

    // wait for thread to load img just in case it is not done
    load_img_1.wait();
#else
    // load image now
    img_loader_1.load_file(img_path_1);    
#endif

    if (img_loader_1.data() == nullptr) {
        std::cout << "[main] Error: Failed to load image from " << img_path_1 << "\n";
    }
    else {
        // jpg -> GL_RGB
        // png -> GL_RGBA
        // refer to https://docs.gl/gl4/glTexStorage2D for internal format

        // define texture2d allocates fixed size gpu memory for texture
        tex1.define_texture2d(1, GL_RGB4, img_loader_1.width(), img_loader_1.height());
        
        // sub image 2d fills the allocated memory
        // 0 (first) is the layer, 0 (second) is the x offset, 0 (third) is the y offset,
        // img_loader_1.width() (fourth), img_loader_1.height() (fifth) are the dimensions
        // of the image, GL_RGB (sixth) is the format, GL_UNSIGNED_BYTE (seventh) is the data
        // type of the pointer used to store the image, GL_UNSIGNED_BYTE refers to unsigned char*
        // and img_loader_1.data() (eighth) is the pointer to the data
        tex1.sub_image2d(0, 0, 0, img_loader_1.width(), img_loader_1.height(), GL_RGB, GL_UNSIGNED_BYTE, img_loader_1.data());

        // generates mipmaps if smaller/larger texture sizes are needed
        tex1.gen_mipmap();
    }

#if WRAP_G_BACKGROUND_RESOURCE_LOAD
    // wait for thread to load img just in case it is not done
    load_img_2.wait();
#else
    // load image now
    img_loader_2.load_file(img_path_2, true);
#endif

    if (img_loader_2.data() == nullptr) {
        std::cout << "[main] Error: Failed to load image from " << img_path_2 << "\n";
    }
    else {
        // jpg -> GL_RGB
        // png -> GL_RGBA
        // refer to https://docs.gl/gl4/glTexStorage2D for internal format

        // define texture2d allocates fixed size gpu memory for texture
        tex2.define_texture2d(1, GL_RGBA4, img_loader_2.width(), img_loader_2.height());

        // sub image 2d fills the allocated memory
        // 0 (first) is the layer, 0 (second) is the x offset, 0 (third) is the y offset,
        // img_loader_2.width() (fourth), img_loader_2.height() (fifth) are the dimensions
        // of the image, GL_RGB (sixth) is the format, GL_UNSIGNED_BYTE (seventh) is the data
        // type of the pointer used to store the image, GL_UNSIGNED_BYTE refers to unsigned char*
        // and img_loader_2.data() (eighth) is the pointer to the data
        tex2.sub_image2d(0, 0, 0, img_loader_2.width(), img_loader_2.height(), GL_RGBA, GL_UNSIGNED_BYTE, img_loader_2.data());
        // generates mipmaps if smaller/larger texture sizes are needed
        tex2.gen_mipmap();
    }

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

                // using radial coords in 3d to calculate the new forward direction
                camera_yaw += cursor_offset.x;
                camera_pitch += cursor_offset.y;
                camera_pitch = glm::clamp(camera_pitch, -89.0f, 89.0f);

                camera_dir = glm::normalize(glm::vec3{
                    glm::cos(glm::radians(camera_yaw)) * glm::cos(glm::radians(camera_pitch)),
                    glm::sin(glm::radians(camera_pitch)),
                    glm::sin(glm::radians(camera_yaw)) * glm::cos(glm::radians(camera_pitch))
                });
                
                // calculating other axes
                camera_right = glm::normalize(glm::cross(camera_dir, world_up));
                camera_up = glm::normalize(glm::cross(camera_right, camera_dir));

                // calculate view matrix
                view = glm::lookAt(camera_pos, camera_pos + camera_dir, glm::vec3{0.0f, 1.0f, 0.0f});
            }

            // update last cursor position
            last_cursor = glm::vec2{cursor.first, cursor.second};
        }

        // movement

        // move left
        if (win.get_key(GLFW_KEY_A) == GLFW_PRESS)
        {
            auto offest = - camera_right * movement_sens * dt;
            camera_pos += offest;
            view = glm::translate(view, - offest);
        }

        // move right
        if (win.get_key(GLFW_KEY_D) == GLFW_PRESS)
        {
            auto offset = camera_right * movement_sens * dt;
            camera_pos += offset;
            view = glm::translate(view, - offset);
        }
        
        // move forward
        if (win.get_key(GLFW_KEY_W) == GLFW_PRESS)
        {
            auto offset = camera_dir * movement_sens * dt;
            camera_pos += offset;
            view = glm::translate(view, - offset);
        }

        // move backward
        if (win.get_key(GLFW_KEY_S) == GLFW_PRESS)
        {
            auto offset = - camera_dir * movement_sens * dt;
            camera_pos += offset;
            view = glm::translate(view, - offset);
        }
        
        // move upwards
        if (win.get_key(GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            auto offset = camera_up * movement_sens * dt;
            camera_pos += offset;
            view = glm::translate(view, - offset);
        }

        // move downwards
        if (win.get_key(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            auto offset = - camera_up * movement_sens * dt;
            camera_pos += offset;
            view = glm::translate(view, - offset);
        }
        prog.set_uniform_mat<4>(view_loc, glm::value_ptr(view));

        // press M to make the second image more visible
        // press M with shift to make first image more visible
        if (win.get_key(GLFW_KEY_M) == GLFW_PRESS)
        {
            tex_mix += (win.get_key(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ? -1.0 : 1.0) * tex_mix_sens * dt;
            prog.set_uniform<float>(tex_mix_loc, tex_mix);
        }

        // zoom event
        if (win.get_key(GLFW_KEY_Z) == GLFW_PRESS)
        {
            fov -= (win.get_key(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ? -1.0 : 1.0) * zoom_sens * dt;
            fov = glm::clamp(fov, 1.0f, 45.0f);
            proj = glm::perspective(glm::radians(fov), (float)win.width()/win.height(), 0.1f, 100.0f);
            prog.set_uniform_mat<4>(proj_loc, glm::value_ptr(proj));
        }

        // * Reset all variables to their original values
        // * This includes position and cursor position and tex mix
        if (win.get_key(GLFW_KEY_R) == GLFW_PRESS)
        {
            glfwSetCursorPos(win.win(), (double)win.width() / 2.0, (double)win.height() / 2.0);
            first_mouse = true;
            camera_pos = camera_start_pos;
            camera_dir = camera_start_looking_at - camera_start_pos;
            
            camera_right = glm::normalize(glm::cross(camera_dir, world_up));
            camera_up = glm::normalize(glm::cross(camera_right, camera_dir));
            
            camera_pitch = glm::degrees(glm::asin(camera_dir.y));
            camera_yaw = glm::degrees(glm::asin(camera_dir.z / glm::cos(glm::radians(camera_pitch))));
            view = glm::lookAt(camera_start_pos, camera_start_looking_at, glm::vec3{0.0f, 1.0f, 0.0f});
            
            prog.set_uniform_mat<4>(view_loc, glm::value_ptr(view));

            fov = starting_fov;
            proj = glm::perspective(glm::radians(fov), (float)win.width()/win.height(), 0.1f, 10.0f);
            prog.set_uniform_mat<4>(proj_loc, glm::value_ptr(proj));

            tex_mix = starting_tex_mix;
            prog.set_uniform<float>(tex_mix_loc, tex_mix);
        }

        watch.start();

        ////
        // rendering

        // set the color that will be used when glClear is called on the color buffer bit
        glClearColor(blue.r, blue.g, blue.b, blue.a);

        // use this to reset the color and reset the depth buffer bit
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // bind vao and shader program before issuing draw call
        vao.bind();
        prog.use();

        for (size_t i = 0; i < cube_positions.size(); ++i)
        {
            model = glm::translate(glm::mat4{1.0f}, cube_positions[i]);
            prog.set_uniform_mat<4>(model_loc, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, verts.size());
        }
        
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