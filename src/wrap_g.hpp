/**
 * @file wrap_g.hpp
 * @author Rashnan
 * @brief A simple wrapper class for using opengl
 * @version 0.1
 * @date 2022-07-15
 *
 * @copyright Copyright (c) 2022
 *
 */

// ensure functions are only declared once
#ifndef WRAP_G_HPP
#define WRAP_G_HPP

////
// Controls

// the opengl version to build wrap_g for
// the default version is 4.6
// min supported version will be 3.3
#ifndef WRAP_G_OPENGL_VERSION_MAJOR
#define WRAP_G_OPENGL_VERSION_MAJOR 4
#endif

#ifndef WRAP_G_OPENGL_VERSION_MINOR
#define WRAP_G_OPENGL_VERSION_MINOR 6
#endif

// whether background resource loading threads should be used
#ifndef WRAP_G_BACKGROUND_RESOURCE_LOAD
#define WRAP_G_BACKGROUND_RESOURCE_LOAD true
#endif


// whether debug logs should be included
#ifndef WRAP_G_DEBUG
#define WRAP_G_DEBUG true
#endif

// TODO: add things for opengl 3.3 + support

// whether to use opengl 4.3+ glDebugMessageControl and etc.
// TODO: test in more advanced scenarios
#ifndef WRAP_G_USE_NEW_OPENGL_DEBUG_MESSAGE_CONTROL
#define WRAP_G_USE_NEW_OPENGL_DEBUG_MESSAGE_CONTROL false
#endif

////
// Imports

// stl
#include <iostream>
#include <unordered_map>
#include <vector>

// gl
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// local
#include "utils.hpp"

// TODO: comments in hpp and impl hpp file

namespace wrap_g
{

    ////////
    // forward declarations
    ////////

    class wrap_g;
    class window;
    class vertex_array_object;
    class program;
    class texture;

    ////////
    // concepts
    ////////

    template <typename Fn>
    concept FramebufferSizeCallback = requires(Fn fn, GLFWwindow *win, GLint w, GLint h)
    {
        fn(win, w, h);
    };

    template <typename Fn>
    concept KeyCallback = requires(Fn fn, GLFWwindow *win, GLint key, GLint scancode, GLint action, GLint mods)
    {
        fn(win, key, scancode, action, mods);
    };

    template<typename Fn>
    concept CursorPositionCallback = requires(Fn fn, GLFWwindow *win, GLdouble x, GLdouble y)
    {
        fn(win, x, y);
    };

    template<typename Fn>
    concept ScrollCallback = requires(Fn fn, GLFWwindow *win, GLdouble dx, GLdouble dy)
    {
        fn(win, dx, dy);
    };

    template <typename Fn>
    concept MouseButtonCallback = requires(Fn fn, GLFWwindow *win, int button, int action, int mods)
    {
        fn(win, button, action, mods);
    };

    ////////
    // declarations
    ////////

    ////
    // wrap_g

    /**
     * @brief A simple wrapper for opengl functions with glfw and glad. Can only be initialized for one version
     * of opengl.
     * * In general most glfw callbacks will be from wrap_g or from window
     *
     */
    class wrap_g
    {
    public:
        // the major opengl version
        static constexpr const unsigned int opengl_version_major = WRAP_G_OPENGL_VERSION_MAJOR;

        // the minor opengl version
        static constexpr const unsigned int opengl_version_minor = WRAP_G_OPENGL_VERSION_MINOR;

    private:
        // the output stream for debug and error logging
        std::ostream &m_out;

        // a vairable that defines whether glfw is initialized
        bool m_init = false;

    public:
        /**
         * @brief Initialize glfw. The version of opengl should be provided via a define (Done to prevent
         * wrap_g from needing template arguments). Remove code from other opengl versions that arent being
         * built for.
         *
         * @param out The stream to output logs
         */
        wrap_g(std::ostream &out = std::cout) noexcept;

        /**
         * @brief Terminate glfw. Remove all allocated resourecs by glfw.
         *
         */
        ~wrap_g() noexcept;

        [[nodiscard]] inline constexpr std::ostream &out() noexcept { return m_out; }
        [[nodiscard]] inline constexpr bool valid() const noexcept { return m_init; }

        /**
         * @brief Create a window object with this graphics object.
         *
         * @param width The width of the window.
         * @param height THe height of the window.
         * @param title The title that should be shown at the top of the window.
         * @param fullscreen Whether the window should be shown in fullscreen.
         * @return window The window object.
         */
        window create_window(GLint width, GLint height, const GLchar *title, bool fullscreen = false) noexcept;

        /**
         * @brief Create a window object with this graphics object and a shared resources window. VAOs and programs
         * created with this window can also be accessed by the shared resources window and vice versa.
         *
         * @param width The width of the window.
         * @param height THe height of the window.
         * @param title The title that should be shown at the top of the window.
         * @param window The window with which resources should be shared.
         * @param fullscreen Whether the window should be shown in fullscreen.
         * @return window The window object.
         */
        window create_window(GLint width, GLint height, const GLchar *title, const window &win, bool fullscreen = false) noexcept;
    };

    ////
    // window

    /**
     * @brief A simple window object.
     * * In general most glfw callbacks will be from wrap_g or from window
     *
     */
    class window
    {
    private:
        wrap_g &__graphics;

        GLint m_width = 0;
        GLint m_height = 0;
        const GLchar *m_title = "\0";
        GLFWwindow *m_win = nullptr;

    public:
        /**
         * @brief Prevent window from being constructed from anywhere other than through
         * the graphics object.
         *
         */
        window() = delete;

        /**
         * @brief Destroy the window.
         *
         */
        ~window() noexcept;

    private:
        /**
         * @brief Construct a new window object.
         *
         * @param __graphics The graphics object being used.
         * @param width The width of the window.
         * @param height THe height of the window.
         * @param title The title that should be shown at the top of the window.
         * @param fullscreen Whether the window should be shown in fullscreen.
         */
        window(wrap_g &__graphics, GLint width, GLint height, const GLchar *title, bool fullscreen = false) noexcept;

        /**
         * @brief Construct a new window object
         *
         * @param __graphics The graphics object being used.

         * @param width The width of the window.
         * @param height THe height of the window.
         * @param title The title that should be shown at the top of the window.
         * @param window The window with which resources should be shared.
         * @param fullscreen Whether the window should be shown in fullscreen.
         */
        window(wrap_g &__graphics, GLint width, GLint height, const GLchar *title, const window &win, bool fullscreen = false) noexcept;

    public:
        [[nodiscard]] inline constexpr GLFWwindow *win() const noexcept { return m_win; }
        [[nodiscard]] inline constexpr GLint width() const noexcept { return m_width; }
        [[nodiscard]] inline constexpr GLint height() const noexcept { return m_height; }
        [[nodiscard]] inline constexpr const GLchar *title() const noexcept { return m_title; }

        // check whether glad has been initialized.
        [[nodiscard]] inline bool check_glad() const noexcept { return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); }

        // gets whether the window should be closed.
        [[nodiscard]] inline bool get_should_close() const noexcept { return glfwWindowShouldClose(m_win); }

        // get the current position of the cursor.
        [[nodiscard]] inline std::pair<double, double> get_cursor_position() const noexcept;

        /**
         * @brief get the mouse button from the window.
         * @param button the mouse button in question ex: GLFW_MOUSE_BUTTON_LEFT
        */
        [[nodiscard]] inline int get_mouse_button(int button) const noexcept { return glfwGetMouseButton(m_win, button); }

        /**
         * @brief gets whether a specific key in the keyboard was pressed.
         * @param key the key to check
         */
        [[nodiscard]] inline int get_key(int key) const noexcept { return glfwGetKey(m_win, key); }

        /**
         * @brief Set the framebuffer size callback.
         *
         * @tparam Fn The type of the framebuffer callback. It must satisfy the FramebufferSizeCallback
         * concept; be a functions that accepts GLFWwinddow* window, int width, int height as its parameters
         * and returns void.
         * * This param should not be set manually and will be deduced by the compiler.
         * @param fn The framebuffer callback.
         */
        template <typename Fn>
        requires FramebufferSizeCallback<Fn>
        void set_framebuffer_size_callback(Fn fn) noexcept;

        /**
         * @brief Set the key callback.
         *
         * @tparam Fn The type of the framebuffer callback. It must satisfy the KeyCallback concept;
         * be a function that accpets GLFWwindow* window, int key, int scancode, int action, int mods and
         * returns void.
         * @param fn The framebuffer callback.
         */
        template <typename Fn>
        requires KeyCallback<Fn>
        void set_key_callback(Fn fn) noexcept;

        /**
         * @brief Set the cursor position callback.
         * @tparam Fn The type of the cursor position callback. It must fulfill the CursorPositionCallback
         * concept; be a function that accepts GLFWwindow* window, double x, double y and returns void.
         * @param fn The cursor position callback.
        */
        template <typename Fn>
        requires CursorPositionCallback<Fn>
        void set_cursor_position_callback(Fn fn) noexcept;

        /**
         * @brief Set the mouse button callback.
         * @tparam Fn The type of the mouse button position callback. It must fulfill the MouseButtonCallback
         * concept; be a function that accepts GLFWwindow* window, int button, int action, int mods and returns void.
         * @param fn The mouse position callback.
        */
        template <typename Fn>
        requires MouseButtonCallback<Fn>
        void set_mouse_button_callback(Fn fn) noexcept;

        /**
         * @brief Set whether the window should close.
         *
         * @param close Whether the window should close.
         */
        void set_should_close(bool close) noexcept;

        /**
         * @brief Make this window the current context. Context is independant for each thread.
         * Each thread can only have one context. A current context is required for programs and
         * vaos to be created in that thread.
         */
        void set_current_context() noexcept;

        /**
         * @brief Swap the buffers of the window. All draw calls draw on a hidden buffer. This
         * hidden buffer should be swapped with the currently viewed buffer when all draw calls
         * are finished so that frame rendering seems instantaneous.
         */
        void swap_buffers() noexcept;

        /**
         * @brief Set the buffer swap interval object. The window will automatically swap when the
         * time has passed. Set to 0 to enable vsync.
         */
        void set_buffer_swap_interval(int interval) noexcept;

        /**
         *
         * @brief Create a vao object. Used from opengl 3 onwards to store vertices and other parameters
         * used in fragment, vertex shaders etc. define_attrib must be called first to define at least one
         * attribute to a specific location which will be used in a shader.Then create array buffer should
         * be called to assign the buffer to a specific location. Create an element array buffer after
         * defining attributes which can be used to storethe array which contains indices for glDrawElements
         * and similar draw calls. Then bind the vao and use glDrawArrays or other draw calls to draw shapes.
         *
         * @return vertex_array_object
         */
        vertex_array_object create_vao() noexcept;

        /**
         * @brief Create a program object. The program can be used to create shaders such as fragment
         * First create_shader should be called to compile and attach a shader to the current program.
         * Once all the shaders are created and attached, link_shaders should be called. At each of these
         * stages if an error occurs it will be output into the graphics object's output stream. After
         * linking all the shaders, call use to use the program. Uniforms within shaders can be set
         * before shader is used for opengl 4+ and for opengl 3 setting a uniform uses the program within
         * the use function itself. Obtaining uniform location is heavy so uniform location should be used to
         * locate and store uniform locations so they can be reused later.
         *
         * @return program
         */
        program create_program() noexcept;

        /**
         * @brief Create a texture object. First create a texture for a specific target ex: GL_TEXTURE_2D.
         * Then bind the texture to a specific texture unit. Max no. of texture units is independant for
         * each system but opengl has a minimum limit of 16. Then parameters of the texture should be set.
         * Typical parameters include:
         * GL_TEXTURE_WRAP_R/S/T which defines how the texture should be wrapped
         * around the x/y/z axis respectively, GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER which can be
         * set to GL_LINEAR, GL_NEAREST, GL_LINEAR_MIPMAP_NEAREST and GL_LINEAR_MIPMAP_LINEAR. Use GL_LINEAR
         * for more realism and GL_NEAREST for a pixelated view. The MIPMAP variants compare the current pixel
         * with the nearest mipmap pixel.
         * After parameters are set, call define_texture2d which will create the storage to store the image
         * in the gpu (storage cannot be changed) and call sub_image2d to assign image data to the previously
         * created storage and call gen_mipmap to create mipmaps.
         * When assigning textures as sampler uniforms to shaders, pass the int index of the texture unit as the
         * uniform param
         *
         * @param target The target texture. Ex: GL_TEXTURE_2D for 2D textures.
         * @return texture
         */
        texture create_texture(GLenum target) noexcept;

        friend class wrap_g;
    };

    ////
    // vertex array object

    /**
     * @brief Used from opengl 3 onwards to store vertices and other parameters
     * used in fragment, vertex shaders etc. define_attrib must be called first to define at least one
     * attribute to a specific location which will be used in a shader.Then create array buffer should
     * be called to assign the buffer to a specific location. Create an element array buffer after
     * defining attributes which can be used to storethe array which contains indices for glDrawElements
     * and similar draw calls. Then bind the vao and use glDrawArrays or other draw calls to draw shapes.
     */
    class vertex_array_object
    {
    public:
        struct array_buffer
        {
            GLuint buffer_id;
            size_t container_size;
        };

    private:
        wrap_g &__graphics;

        GLuint m_id = 0;

        std::unordered_map<GLuint, array_buffer> m_array_buffers;
        GLuint m_element_buffer_id = 0;

    public:
        /**
         * @brief Disally vaos from being created without a window.
         *
         */
        vertex_array_object() = delete;

        /**
         * @brief Destroy the vertex array object and all allocated sub-resources
         *
         */
        ~vertex_array_object() noexcept;

    private:
        /**
         * @brief Create a vao object.
         *
         * @param __graphics The graphics object being used.
         */
        vertex_array_object(wrap_g &__graphics) noexcept;

    public:
        [[nodiscard]] inline constexpr GLuint id() const noexcept { return m_id; }

        /**
         * @brief Create an array buffer object.
         *
         * @tparam Wrapper The Wrapper container which contains all the data for each vertex.
         * @tparam Stride The stride is the distance between adjacent vertex data. It is
         * usually considered to be the sizeof the Wrapper container but there are cases
         * where it is necessary to be changed, ex: stride is 0 if all vertices should use the
         * same data points. ex: faces of a cube should have same texture.
         * @param binding_index The index which the buffer should be bound to.
         * @param buffer_size The total size of the buffer in bytes.
         * @param data The pointer to the underlying data of the buffer.
         * @param flags The flags that should be set on the buffer data. GL_MAP_READ_BIT allows read
         * and GL_MAP_WRITE_BIT allows writing. The parameters are generally same as glNamedBufferStorage:
         * https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferStorage.xhtml
         * @param offset The offset at which the data is from the pointer provided.
         */
        template <typename Wrapper>
        void create_array_buffer(GLuint binding_index, GLsizeiptr buffer_size, Wrapper *data, GLbitfield flags, GLintptr offset = 0) noexcept;

        /**
         * @brief Create an array buffer object.
         *
         * @tparam Wrapper The Wrapper container which contains all the data for each vertex.
         * @param binding_index The index which the buffer should be bound to.
         * @param buffer_size The total size of the buffer in bytes.
         * @param data The pointer to the underlying data of the buffer.
         * @param stride The stride is the distance between adjacent vertex data. It is
         * usually considered to be the sizeof the Wrapper container but there are cases
         * where it is necessary to be changed, ex: stride is 0 if all vertices should use the
         * same data points. ex: faces of a cube should have same texture.
         * @param flags The flags that should be set on the buffer data. GL_MAP_READ_BIT allows read
         * and GL_MAP_WRITE_BIT allows writing. The parameters are generally same as glNamedBufferStorage:
         * https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferStorage.xhtml
         * @param offset The offset at which the data is from the pointer provided.
         * ! important: offset is compulsory to prevent ambiguousness with other variant
         */
        template <typename Wrapper>
        void create_array_buffer(GLuint binding_index, GLsizeiptr buffer_size, Wrapper *data, GLsizei stride, GLbitfield flags, GLintptr offset) noexcept;

        /**
         * @brief Create a element buffer object.
         *
         * @tparam Wrapper The wrapper conatainer which contains all the data for each object. Ex: GL_TRIANGLES
         * need 3 ints.
         * @param buffer_size The total size of the buffer in bytes.
         * @param data The pointer to the underlying data of the buffer.
         * @param flags The flags that should be set on the buffer data. GL_MAP_READ_BIT allows read
         * and GL_MAP_WRITE_BIT allows writing. The parameters are generally same as glNamedBufferStorage:
         * https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferStorage.xhtml
         */
        template <typename Wrapper>
        void create_element_buffer(GLsizeiptr buffer_size, Wrapper *data, GLbitfield flags) noexcept;

        /**
         * @brief Define an attribute which will bebound to a speciic location and used within a shader.
         *
         * @param binding_index The index at which the buffer containing the attribute data is located at.
         * @param attrib_index The index of the attribute.
         * @param count The number of items of data for each vertex. (3 floats for position so count is 3.)
         * @param data_type The data type of the data. (3 float for position so data type is GL_FLOAT.)
         * @param normalised Whether the data should be normalised.
         * @param relative_offset The relative offset at which the attribute data is located. Ex: a
         * wrapper containing 3 floats for position and 3 floats for color, the relative offset for position
         * is 0 and for color is 3 * sizeof(float).
         */
        void define_attrib(GLuint binding_index, GLuint attrib_index, GLint count, GLenum data_type, bool normalised = false, GLuint relative_offset = 0) noexcept;

        /**
         * @brief Bind the vao. The vao must be bound before using it for draw calls.
         *
         */
        void bind() const noexcept;

        friend class window;
    };

    ///
    // program

    /**
     * @brief The program can be used to create shaders such as fragment
     * First create_shader should be called to compile and attach a shader to the current program.
     * Once all the shaders are created and attached, link_shaders should be called. At each of these
     * stages if an error occurs it will be output into the graphics object's output stream. After
     * linking all the shaders, call use to use the program. Uniforms within shaders can be set
     * before shader is used for opengl 4+ and for opengl 3 setting a uniform uses the program within
     * the use function itself. Obtaining uniform location is heavy so uniform location should be used to
     * locate and store uniform locations so they can be reused later.
     */
    class program
    {
    private:
        wrap_g &__graphics;

        GLuint m_id;
        // std::unordered_map<GLenum, std::vector<GLuint>> m_shaders;
        std::vector<GLuint> m_shaders;

    public:
        /**
         * @brief Disable programs from being created without a window.
         *
         */
        program() = delete;

        /**
         * @brief Destroy the program and all allocated sub-resources.
         *
         */
        ~program() noexcept;

    private:
        /**
         * @brief Construct a new program object.
         *
         * @param __graphics The graphics object being used.
         */
        program(wrap_g &__graphics) noexcept;

        /**
         * @brief Create a program object based on a pre determined template. This function should be used
         * if all the shaders and their file paths are available immediately
         * * (Code for each repective shader must be stored in files for this function).
         *
         * @tparam String The type of the string object provided. Ex: std::string, const char *, std::string_view
         * @param __graphics The graphics object used *provided by the window object used to create this program.
         * @param shaders An unordered map containing a pair of a shader type and a vector of strings
         * that indicate the file path for each respective shader type or a String containgin the shader source code.
         * ! Not tested. Multiple shaders of same type allowed.
         * ! all strings must have null terminator
         * 
         */
        template<typename String = std::string>
        requires utils::Stringable<String>
        program(wrap_g &__graphics, const std::unordered_map<GLenum, std::vector<String>> &shaders) noexcept;

    public:
        [[nodiscard]] inline constexpr GLuint id() const noexcept { return m_id; }

        /**
         * @brief Load and store shader source code.
         * 
         */

        /**
         * @brief Quickly create and link a group of shaders
         * 
         * @tparam String The type of the string object provided. Ex: std::string, const char *, std::string_view
         * @param shaders An unordered map containing a pair of a shader type and a vector of strings
         * that indicate the file path for each respective shader type or a String containgin the shader source code.
         * @return true Created & linked shaders successfully.
         * @return false Failed to create shaders or link successfully.
         */
        template<typename String = std::string>
        requires utils::Stringable<String>
        [[nodiscard]] bool quick(const std::unordered_map<GLenum, std::vector<String>> &shaders) noexcept;

        /**
         * @brief Creates, compiles and attaches a shader object. The shader objec can be a vertex shader or a fragment shader etc.
         *
         * @param shader_type The type of shader to be created. Ex: GL_FRAGMENT_SHADER, GL_VERTEX_SHADER.
         * @param code The code for the shader.
         * @return true Created shader successfully.
         * @return false Failed to create shader.
         */
        bool create_shader(GLenum shader_type, const char *code) noexcept;

        /**
         * @brief Link all the shaders that are currently attached to the program.
         *
         * @return true Linked shader successfully.
         * @return false Failed to link shader.
         */
        bool link_shaders() noexcept;

        /**
         * @brief Use the current program.
         * * Must be called before draw calls.
         *
         */
        void use() const noexcept;

        void flush_shaders() noexcept;

        /**
         * @brief Get the uniform location of a uniform in a shader in this program. For opengl 3
         * the shader must be used first ( call program.use(); ). For opengl 4 the uniform can be updated directly.
         * The uniform location is heavy to search and should therefore be cached.
         *
         * @param name The name of the uniform.
         * @return int The location of the uniform.
         */
        template<typename String>
        requires utils::Stringable<String>
        int uniform_location(String name) const noexcept;

        // TODO: check if array version is better
        /**
         * @brief Get the uniform location of multiple unifomrs in a shader in this program.
         * ! all strings must have null terminator
         */
        template <typename... Ts>
        requires((utils::Stringable<Ts> && ...))
        std::vector<int> uniform_locations(Ts &&...names) const noexcept;

        // /**
        // //  * @brief Get the uniform location of multiple unifomrs in a shader in this program.
        // // * ! all strings must have null terminator
        // // */
        // template <typename... Ts>
        // requires((utils::Stringable<Ts> && ...))
        // std::array<int, sizeof...(Ts)> uniform_locations(Ts &&...names)
        // const noexcept;

        /**
         * @brief Set the value of a uniform in the program. This function can be used to set the uniform
         * of an unsigned int, int, float, double and their respective 2, 3, and 4 item vectors. set_uniform_vec
         * can also be used to set vector uniforms using count = 1.
         *
         * @param loc The location of the uniform. Use uniform_location to find the location using name.
         * @param vals The values.
         */
        template <typename... Ts>
        requires(std::is_integral_v<Ts> &&...) || (std::is_floating_point_v<Ts> && ...)
        void set_uniform(int loc, const Ts &...vals) noexcept;

        /**
         * @brief Set the value of a vector uniform. This function can be used to set the vector uniform of many
         * unsigned int, int, float, and double vectors of size 2, 3, 4.
         *
         * @tparam vec_size THe size of the vector.
         * @tparam T The data type of the vector.
         * @param loc The location of the uniform. Use uniform_location to find the location using name.
         * @param val A pointer to the vectors which must be consecutively stored.
         * @param count The count of vectors in the uniform.
         */
        template <size_t vec_size, typename T = float>
        requires std::is_floating_point_v<T>
        void set_uniform_vec(int loc, T *val, size_t count = 1) noexcept;

        /**
         * @brief Set the value of a vector matrix. This function can be used to set the matrix uniform of float
         * and double matrices of 2x2, 2x3, 2x4, 3x2, 3x3, 3x4, 4x2, 4x3, 4x4.
         *
         * @tparam rows The rows in the matrix.
         * @tparam cols The cols in the matrix. If not set, it is set to be equal to the number of rows automatically.
         * @tparam T The data type of the data in the matrix.
         * @param loc The location of the uniform. Use uniform_location to find the location using name.
         * @param val A pointer to the matrices and their data which must be consecutively stored.
         * @param count The count of vectors to be stored in the uniform.
         * @param transpose Whether the matrices should be transposed. If true all matrices are transposed and if false
         * none are transposed. Default set to false.
         */
        template <size_t rows, size_t cols = rows, typename T = float>
        requires std::is_floating_point_v<T>
        void set_uniform_mat(int loc, T *val, size_t count = 1, bool transpose = false) noexcept;

        friend class window;
    };

    ////
    // texture

    /**
     * @brief First create a texture for a specific target ex: GL_TEXTURE_2D.
     * Then bind the texture to a specific texture unit. Max no. of texture units is independant for
     * each system but opengl has a minimum limit of 16. Then parameters of the texture should be set.
     * Typical parameters include:
     * GL_TEXTURE_WRAP_R/S/T which defines how the texture should be wrapped
     * around the x/y/z axis respectively, GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER which can be
     * set to GL_LINEAR, GL_NEAREST, GL_LINEAR_MIPMAP_NEAREST and GL_LINEAR_MIPMAP_LINEAR. Use GL_LINEAR
     * for more realism and GL_NEAREST for a pixelated view. The MIPMAP variants compare the current pixel
     * with the nearest mipmap pixel.
     * After parameters are set, call define_texture2d which will create the storage to store the image
     * in the gpu (storage cannot be changed) and call sub_image2d to assign image data to the previously
     * created storage and call gen_mipmap to create mipmaps.
     * When assigning textures as sampler uniforms to shaders, pass the int index of the texture unit as the
     * uniform param
     */
    class texture
    {
    private:
        wrap_g &__graphics;

        GLuint m_id = 0;
        GLenum m_target;

    public:
        /**
         * @brief Disable texture objects from being made without a window.
         *
         */
        texture() = delete;

        /**
         * @brief Destroy the texture object and all allocated sub-resources.
         *
         */
        ~texture() noexcept;

    private:
        /**
         * @brief Construct a new texture object.
         *
         * @param __graphics The graphics object which is being used.
         * @param target The target to which the texture object should be created for.
         */
        texture(wrap_g &__graphics, GLenum target) noexcept;

    public:
        [[nodiscard]] inline constexpr GLuint id() const noexcept { return m_id; }
        [[nodiscard]] inline constexpr GLenum target() const noexcept { return m_target; }

        void recreate() noexcept;

        /**
         * @brief Bind the texture to a specific texture unit.
         *
         * @param texture_unit The unit to bind to. The min is 0 and the max is determined by the system.
         * Opengl has a lowest maximum which is defined as 16.
         */
        void bind_unit(GLuint texture_unit) const noexcept;

        /**
         * @brief Set a parameter of the texture.
         *
         * @param paran The parameter.
         * @param val The value it should be set to.
         */
        template <typename T>
        requires std::is_integral_v<T> || std::is_floating_point_v<T>
        void set_param(GLenum paran, const T &val) noexcept;

        /**
         * @brief Set a vector parameter of the texture.
         *
         * @param paran The parameter.
         * @param arr A pointer to the array of values which is its value.
         */
        template <typename T, typename ForceType = T>
        requires std::is_integral_v<T> || std::is_floating_point_v<T>
        void set_param_vec(GLenum param, const T *arr) noexcept;

        /**
         * @brief Allocate the storage within the gpu to store a 2d image. This allocated storage cannot
         * be changed.
         *
         * @param levels The maximum amount of levels.
         * @param internal_format The internal format of data. Ex: GL_RGBA4 to use 4 bytes to store red
         * green and blue each. Refer to https://docs.gl/gl4/glTexStorage2D
         * 
         * @param width The width of the texture.
         * @param height the height of the texture.
         */
        void define_texture2d(size_t levels, GLenum internal_format, size_t width, size_t height) noexcept;

        /**
         * @brief Assign data to a block of data alloacted on the gpu with define_texture2d. This function
         * must be used to set the image data.
         *
         * @param level The level of the image.
         * @param xoffset The x offset.
         * @param yoffset The y offset.
         * @param width The width of the image.
         * @param height The height of the image.
         * @param format The format of the image.
         * @param type The data type of the image data. Ex: GL_UNSIGNED_BYTE
         * @param pixels A pointer to the image data.
         */
        void sub_image2d(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) noexcept;

        /**
         * @brief Create mipmaps for the texture.
         *
         */
        void gen_mipmap() noexcept;

        friend class window;
    };

} // namespace wrap_g

#include "wrap_g_impl.hpp"

#endif