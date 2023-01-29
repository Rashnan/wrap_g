#ifndef WRAP_G_TESTS_TEXTURED_RECT
#define WRAP_G_TESTS_TEXTURED_RECT

#include <iostream>
#include <future>

#include "../src/utils.hpp"
#include "../src/wrap_g.hpp"

namespace wrap_tests
{
    
void create_textured_rect() noexcept
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
    // Resource locations

    const char *img_path_1 = "tests/res/images/wall.jpg";
    const char *img_path_2 = "tests/res/images/awesomeface.png";

    utils::stb_image img_loader_1;
    utils::stb_image img_loader_2;

    const char *vert_path = "tests/res/shaders/textured_rect.vs";
    const char *frag_path = "tests/res/shaders/textured_rect.fs";

#if WRAP_G_MULTITHREADING
    ////
    // background resource fetching

    // call blocking functions such as files/img loading in seperate thread.

    auto load_img_1 = std::async(std::launch::async, [img_path_1, &img_loader_1](){
        img_loader_1.load_file(img_path_1);
    });

    auto load_img_2 = std::async(std::launch::async, [img_path_2, &img_loader_2](){
        stbi_set_flip_vertically_on_load_thread(true);
        // setting vertical_flip param in load_file does not do anything if on seperate thread.
        img_loader_2.load_file(img_path_2);
    });

    std::string vert_src, frag_src;

    auto load_vert_src = std::async(std::launch::async, [vert_path, &vert_src](){
        vert_src = utils::read_file(vert_path);
    });

    auto load_frag_src = std::async(std::launch::async, [frag_path, &frag_src](){
        frag_src = utils::read_file(frag_path);
    });
#endif

    ////
    // startup code

    // create a vao to store vertices and a shader program that defines how the vertices appear on the window
    auto vao = win.create_vao();
    auto prog = win.create_program();
    auto tex1 = win.create_texture(GL_TEXTURE_2D);
    auto tex2 = win.create_texture(GL_TEXTURE_2D);

    // creates a 3d rectangle like below in compile time
    //-         | *********** | (end)
    //-         | *********** |
    //-         | *********** |
    //-         | *********** |
    //-         | *********** |
    //- (start) | *********** |
    constexpr auto verts = utils::gen_rect_face<3>(glm::vec3{-0.5f, -0.5f, 0.0f}, glm::vec3{0.5f, 0.5f, 0.0f});

    // creates a 2d coord within the texture map for each vertex
    constexpr auto tex_coords = utils::gen_rect_face<2>(glm::vec2{0.0f}, glm::vec2{1.0f});

    // mapping the vertices to faces
    // each uvec3 corresponds to a triangle face
    constexpr auto indices = std::array{
        glm::uvec3{
            utils::GEN_RECT_FACE_VERTS::BOTTOM_LEFT,
            utils::GEN_RECT_FACE_VERTS::TOP_LEFT,
            utils::GEN_RECT_FACE_VERTS::TOP_RIGHT,
        },
        glm::uvec3{
            utils::GEN_RECT_FACE_VERTS::BOTTOM_LEFT,
            utils::GEN_RECT_FACE_VERTS::TOP_RIGHT,
            utils::GEN_RECT_FACE_VERTS::BOTTOM_RIGHT
        }
    };

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
    // and the pointer to it is verts.cbegin() (third) and the flag enabled is GL_MAP_READ_BIT (fourth)
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
    vao.create_array_buffer<const glm::vec3>(0, verts.size() * sizeof(glm::vec3), verts.cbegin(), GL_MAP_READ_BIT);
    // creates the buffer for the texture coordinates
    vao.create_array_buffer<const glm::vec2>(1, tex_coords.size() * sizeof(glm::vec2), tex_coords.cbegin(), GL_MAP_READ_BIT);

    // creates an element array buffer of size indices.size() * sizeof(glm::uvec3) (first)
    // and the pointer to it is indices.cbegin() (second) and the flag enabled is GL_MAP_READ_BIT (third)
    // offset from pointer is 0 (default fourth)
    // underlying:
    // - creates an element array buffer
    // - creates fixed size storage (opengl 4.5+)
    // - creates variable size storage (opengl 3.3+ & 4.3+)
    // - binds the buffer to the vertex array object
    // - stores id of buffer to delete at destructor
    // * template parameter is used to tell the type of the data pointer as this is often known
    // * in compile time. Setting this parameter explicitly is not importatnt as it is not used
    // * for any purpose in the uderlying functions in opengl 4.5+ ad opengl 3.3
    // * for opengl 4.3+ there is a difference but unsure.
    // TODO: update
    vao.create_element_buffer<const glm::uvec3>(indices.size() * sizeof(glm::uvec3), indices.cbegin(), GL_MAP_READ_BIT);

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
    
    // both branches do essentially the same thing
#if !WRAP_G_MULTITHREADING
    // quick method to compile and link provided shader files
    // multiple vertex and fragment shaders can be provided and other types of shaders as well
    // template argument tells whether all provided const char* (the vert_src and frag_src) is 
    // a string containing the shader source or a relative path to the file containing the src
    // if true then uses utils to open and read file
    // if false uses string as glsl code directly
    bool success = prog.quick<true>({
        {GL_VERTEX_SHADER, {vert_path}},
        {GL_FRAGMENT_SHADER, {frag_path}}
    });

    if (!success)
        return;
#else
    // ensure vertex source is loaded
    load_vert_src.wait();

    // create and compile the shader source
    // also attaches it to the shader program
    bool success = prog.create_shader(GL_VERTEX_SHADER, vert_src.c_str());
    
    if (!success)
        return;

    // ensure fragment source is loaded
    load_frag_src.wait();

    // create and compile the shader source
    // also attaches it to the shader program
    success = prog.create_shader(GL_FRAGMENT_SHADER, frag_src.c_str());
    
    if (!success)
        return;
    
    // links all attached shaders
    success = prog.link_shaders();

    if (!success)
        return;
#endif

    // ensure sampler2D uniform is provided as an int and is the same as 
    // the unit the texture is bound to.
    prog.set_uniform<int>(prog.uniform_location("tex1"), 0);

    // ensure sampler2D uniform is provided as an int and is the same as 
    // the unit the texture is bound to.
    prog.set_uniform<int>(prog.uniform_location("tex2"), 1);

    float tex_mix = 0.5f, tex_mix_sens = 0.01f;
    int tex_mix_loc = prog.uniform_location("tex_mix");
    prog.set_uniform<float>(tex_mix_loc, tex_mix);

    // gives a glm::vec4 containing the rgba color values
    constexpr auto blue = utils::hex("#111b24");
    constexpr auto yellow = utils::hex("#d2cb7f");

    // sets vector uniform
    // first template argument determines vector length
    // glm::value_ptr gets the pointer of the glm::vec4
    // in practice use uniform_locations or uniform_location to store location of uniforms
    // at initialization for readily changing uniforms
    prog.set_uniform_vec<4>(prog.uniform_location("col"), glm::value_ptr(yellow));

#if WRAP_G_MULTITHREADING
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

#if WRAP_G_MULTITHREADING
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

    std::cout << "[main] Debug: Starting code time elapsed: " << watch.stop() << " ms \n";

    std::cout << "Starting...\n";

    double total_time = 0.0;
    double last_frame = 0.0;
    int n = 1;

    while (!win.get_should_close())
    {
        watch.start();

        // press S to make the second image more visible
        // press S with shift to make first image more visible
        if (glfwGetKey(win.win(), GLFW_KEY_S) == GLFW_PRESS) {
            tex_mix += (glfwGetKey(win.win(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? -1.0 : 1.0) * tex_mix_sens;
            prog.set_uniform<float>(tex_mix_loc, tex_mix);
        }

        // set the color that will be used when glClear is called on the color buffer bit
        glClearColor(blue.r, blue.g, blue.b, blue.a);
        
        glClear(GL_COLOR_BUFFER_BIT);

        // use this instead of above to enable 3d depth testing
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // glEnable(GL_DEPTH_TEST);

        // bind vao and shader program before issuing draw call
        vao.bind();
        prog.use();
        
        // since we are also using an element array buffer we must use this instead
        // count of indices is already set when we created the element array buffer
        // nullptr (fourth) is used as the pointer to the element array buffer
        // in case we have not already provided it
        glDrawElements(GL_TRIANGLES, indices.size() * sizeof(glm::uvec3) / sizeof(unsigned int), GL_UNSIGNED_INT, nullptr);

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