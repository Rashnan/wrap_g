#ifndef WRAP_G_TESTS_TRIANGLE
#define WRAP_G_TESTS_TRIANGLE

#include <iostream>

#include "../src/utils.hpp"
#include "../src/wrap_g.hpp"

namespace wrap_tests
{

void create_triangle() noexcept
{
    // time each process
    utils::timer watch;
    watch.start();

    // initialize glfw and set opengl version and some stuff
    wrap_g::wrap_g graphics;

    if (!graphics.valid())
        return;

    // create a window / context.
    // width: 800
    // height: 600
    // title: "Triangle Test Window."
    // underlying function also checks to see if glad is valid
    auto win = graphics.create_window(800, 600, "Triangle Test Window.");

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

    std::cout << "[main] Debug: Standard stuff time elapsed: " << watch.stop() << " ms \n";
    watch.start();
    
    ////
    // startup code

    // create a vao to store vertices and a shader program that defines how the vertices appear on the window
    auto vao = win.create_vao();
    auto prog = win.create_program();

    // creates a triangle like below
    //-         |      *      | (end)
    //-         |     ***     |
    //-         |    *****    |
    //-         |   *******   |
    //-         |  *********  |
    //- (start) | *********** |
    constexpr auto verts = utils::gen_tri_face(glm::vec2{-0.5f}, glm::vec2{0.5f});

    // define_attrib defines an attribute in the shader and enables it
    // (**) inside brackets is paramter number
    // defines a 3 (third) float (fourth) attribute to be in attribute position 0 (second)
    // and the buffer to get this attribute from is the buffer at binding position 0 (first)
    // and at an offset 0 (default sixth) from the start of the buffer pointer and is not
    // normalized (default fifth)
    vao.define_attrib(0, 0, 3, GL_FLOAT);

    // creates an array buffer at binding position 0 (first) of size verts.size() * sizeof(glm::vec3) (second)
    // and the pointer to it is verts.cbegin() (third) and the flag enabled is GL_MAP_READ_BIT (fourth)
    // offset from pointer is 0 (default fifth)
    // underlying:
    // - creates buffer
    // - creates fixed size storage (opengl 4.5+)
    // - creates variable size storage (opengl 3.3+ & 4.3+)
    // - binds the buffer to the vertex array object
    // - stores id of buffer to delete at destructor
    // * template parameter is used to tell the stride.. distance between vertices in memory
    // * as this is often known at compile time.
    // ! be careful as this may cause headaches if set incorrectly
    vao.create_array_buffer<const glm::vec3>(0, verts.size() * sizeof(glm::vec3), verts.cbegin(), GL_MAP_READ_BIT);

    // extremely basic shader code written in glsl
    const char *vert_src = "\n#version 450 core\n\nlayout (location = 0) in vec3 pos;\n\nvoid main() {\n    gl_Position = vec4(pos.xyz, 1.0);\n}\0",
                *frag_src= "\n#version 450 core\n\nout vec4 frag_col;\n\nuniform vec4 col;\n\nvoid main()\n{\n    frag_col = col;\n}\0";

    // quick method to compile and link provided shader files
    // multiple vertex and fragment shaders can be provided and other types of shaders as well
    // template argument tells whether all provided const char* (the vert_src and frag_src) is 
    // a string containing the shader source or a relative path to the file containing the src
    // if true then uses utils to open and read file
    // if false uses string as glsl code directly
    bool success = prog.quick<false>({
        {GL_VERTEX_SHADER, {vert_src}},
        {GL_FRAGMENT_SHADER, {frag_src}}
    });

    if (!success)
        return;
    
    // gives a glm::vec4 containing the rgba color values
    constexpr auto blue = utils::hex("#111b24");
    constexpr auto yellow = utils::hex("#d2cb7f");

    // sets vector uniform
    // first template argument determines vector length
    // glm::value_ptr gets the pointer of the glm::vec4
    // in practice use uniform_locations or uniform_location to store location of uniforms
    // at initialization for readily changing uniforms
    prog.set_uniform_vec<4>(prog.uniform_location("col"), glm::value_ptr(yellow));

    std::cout << "Starting...\n";

    std::cout << "[main] Debug: Starting code time elapsed: " << watch.stop() << " ms \n";

    double total_time=0.;
    double last_frame=0.;
    int n=1;

    while (!win.get_should_close())
    {
        watch.start();

        // set the color that will be used when glClear is called on the color buffer bit
        glClearColor(blue.r, blue.g, blue.b, blue.a);
        
        glClear(GL_COLOR_BUFFER_BIT);

        // use this instead of above to enable 3d depth testing
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // glEnable(GL_DEPTH_TEST);

        // bind vao and shader program before issuing draw call
        vao.bind();
        prog.use();
        // start from vertex 0 and go for 3 vertices (verts.size())
        glDrawArrays(GL_TRIANGLES, 0, verts.size());

        last_frame = watch.stop();
        total_time += last_frame;
        ++n;
        std::cout << "[main] Debug: Frame render took " << last_frame << " ms.\n";

        // swap the buffers to show the newly drawn frame
        win.swap_buffers();
        // get events such as mouse input
        glfwPollEvents();
    }

    std::cout << "----------------------------------------------------------------\n";
    std::cout << "[main] Debug: Total frames: " << n << ".\n";
    std::cout << "[main] Debug: Average frame render time: " << total_time / n << " ms.\n";
    std::cout << "[main] Debug: FPS: " << 1e3 * n / total_time << "\n";

    std::cout << "[main] Debug: Running code time elapsed: " << total_time << " ms \n";

    std::cout << "Stopping...\n";
}
    
} // namespace wrap_tests


#endif