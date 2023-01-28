#ifndef WRAP_G_TESTS_TRIANGLE
#define WRAP_G_TESTS_TRIANGLE

#include <iostream>

#include "../src/utils.hpp"
#include "../src/wrap_g.hpp"

namespace wrap_tests
{

void create_triangle() noexcept
{
    utils::timer watch;
    watch.start();

    wrap_g::wrap_g graphics;

    if (!graphics.valid())
        return;

    auto win = graphics.create_window(800, 600, "Triangle Test Window.");

    if (win.win() == nullptr)
        return;
    
    win.set_framebuffer_size_callback([](GLFWwindow *, GLint w, GLint h){ glViewport(0, 0, w, h); });

    win.set_key_callback([](GLFWwindow *win, int key, int, int action, int){
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(win, true);
        }
    });

    win.set_buffer_swap_interval(0);

    std::cout << "[main] Debug: Standard stuff time elapsed: " << watch.stop() << " ms \n";
    watch.start();
    
    ////
    // startup code

    auto vao = win.create_vao();
    auto prog = win.create_program();

    // constexpr auto verts = utils::gen_tri_face(glm::vec2{-0.5}, glm::vec2{0.5});
    constexpr auto verts = utils::gen_tri_face(glm::vec2{-0.5f}, glm::vec2{0.5f});

    vao.define_attrib(0, 0, 3, GL_FLOAT);
    vao.create_array_buffer(0, verts.size() * sizeof(glm::vec3), verts.cbegin(), GL_MAP_READ_BIT);

    const char *vert_src = "\n#version 450 core\n\nlayout (location = 0) in vec3 pos;\n\nvoid main() {\n    gl_Position = vec4(pos.xyz, 1.0);\n}\0",
                *frag_src= "\n#version 450 core\n\nout vec4 frag_col;\n\nuniform vec4 col;\n\nvoid main()\n{\n    frag_col = col;\n}\0";

    bool success = prog.quick<false>({
        {GL_VERTEX_SHADER, {vert_src}},
        {GL_FRAGMENT_SHADER, {frag_src}}
    });

    if (!success)
        return;
    
    constexpr auto blue = utils::hex("#111b24");
    constexpr auto yellow = utils::hex("#d2cb7f");

    prog.set_uniform_vec<4>(prog.uniform_location("col"), glm::value_ptr(yellow));

    std::cout << "Starting...\n";
    
    std::cout << "[main] Debug: Starting code time elapsed: " << watch.stop() << " ms \n";

    double total_time=0.;
    double last_frame=0.;
    int n=1;

    while (!win.get_should_close())
    {
        watch.start();

        glClearColor(blue.r, blue.g, blue.b, blue.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        vao.bind();
        prog.use();

        // utils::print_vecs<3*3>((float *)verts.cbegin());

        glDrawArrays(GL_TRIANGLES, 0, verts.size() * sizeof(glm::vec3) / sizeof(float));

        last_frame = watch.stop();
        total_time += last_frame;
        ++n;
        std::cout << "[main] Debug: Frame render took " << last_frame << " ms.\n";

        win.swap_buffers();
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