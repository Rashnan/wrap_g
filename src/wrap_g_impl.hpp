
/**
 * @file wrap_g_impl.hpp
 * @author Rashnan
 * @brief The implementation for the wrap_g functions
 * @version 0.1
 * @date 2022-07-15
 *
 * @copyright Copyright (c) 2022
 *
 */

// ensure functions are only defined once
#ifndef WRAP_G_IMPL_HPP
#define WRAP_G_IMPL_HPP

// local
#include "wrap_g.hpp"

namespace wrap_g
{

    ////////
    // definitions
    ////////

    ////
    // wrap_g

    wrap_g::wrap_g(std::ostream &out) noexcept
        : m_out(out)
    {
        if (!glfwInit())
        {
            m_out << "[wrap_g] Error: Failed to initialize glfw.\n";
            return;
        }

        // set opengl version
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, opengl_version_major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, opengl_version_minor);

#if WRAP_G_DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
        // set opengl profile
        // opengl version >= 3.3 will always use core profile
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if __APPLE__
        // set forward compatibility true
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


#if WRAP_G_DEBUG
    // set the error callback
    // use the glfw error callback function
    
    glfwSetErrorCallback([](GLint code, const GLchar *msg)
                    { std::cout << "[glfw] Error " << code << ": " << msg << "\n"; });
#endif
        // identify that graphics successfully initialized
        m_init = true;

#if WRAP_G_DEBUG
        m_out << "[wrap_g] Debug: Initialized glfw.\n";
#endif
    }

    wrap_g::~wrap_g() noexcept
    {
        // destroy all the memory allocated
        glfwTerminate();

#if WRAP_G_DEBUG
        m_out << "[wrap_g] Debug: Terminated glfw.\n";
#endif
    }

    window wrap_g::create_window(GLint width, GLint height, const GLchar *title, bool fullscreen) noexcept
    {
        // simply create a window object
        // and reduces the parameters by one
        // no need to manually type the graphics context
        // as one of the function parameters
        // also enforces the fact that wrap_g needs to 
        // be initialized first to properly create
        // a window
        return window(*this, width, height, title, fullscreen);
    }

    window wrap_g::create_window(GLint width, GLint height, const GLchar *title, const window &win, bool fullscreen) noexcept
    {
        // simply create a window object
        // and reduces the parameters by one
        // no need to manually type the graphics context
        // as one of the function parameters
        // also enforces the fact that wrap_g needs to 
        // be initialized first to properly create
        // a window
        return window(*this, width, height, title, win, fullscreen);
    }

    ////
    // window

    window::~window()
    {
        // destroy the window 
        glfwDestroyWindow(m_win);

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Destroyed window.\n";
#endif
    }

    window::window(wrap_g &__graphics, GLint width, GLint height, const GLchar *title, bool fullscreen) noexcept
        : __graphics(__graphics), m_width(width), m_height(height), m_title(title)
    {
        // create the window with the parameters
        m_win = glfwCreateWindow(width, height, title, fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
        
        // check whether the window was actually created
        if (m_win == nullptr)
        {
            __graphics.out() << "[wrap_g] Error: Failed to create window.\n";
            return;
        }
        // make it the current context in this thread
        glfwMakeContextCurrent(m_win);

        // make sure glad is loaded properly
        if (!check_glad())
        {
            __graphics.out() << "[wrap_g] Error: Failed to initialize glad.\n";
            return;
        }

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Created window.\n";
#endif
#if WRAP_G_USE_NEW_OPENGL_DEBUG_MESSAGE_CONTROL
        // check whether debug context was activated
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            // enable the debug output
            glEnable(GL_DEBUG_OUTPUT);

            // make the output synchron6ous
            // so the output commands are called immediately
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

            // allow all debug messages
            // this function can be used to control debug outputs
            // for sources, types, severity
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
            
            // actual debug message output function
            glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei llength, const GLchar* message, const void *user_param){
                std::stringstream ss;

                switch (source)
                {
                case GL_DEBUG_SOURCE_API:
                    ss << "[opengl api] ";
                    break;
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                    ss << "[window system] ";
                    break;
                case GL_DEBUG_SOURCE_SHADER_COMPILER:
                    ss << "[shader compiler] ";
                    break;
                case GL_DEBUG_SOURCE_THIRD_PARTY:
                    ss << "[third party] ";
                    break;
                case GL_DEBUG_SOURCE_APPLICATION:
                    ss << "[application] ";
                    break;
                case GL_DEBUG_SOURCE_OTHER:
                    ss << "[other] ";
                    break;
                case GL_DONT_CARE:
                default:
                    ss << "[unknown] ";
                    break;
                }
                
                switch (type)
                {
                    case GL_DEBUG_TYPE_ERROR:
                        ss << "(Error) ";
                        break;
                    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                        ss << "(Deprecated) ";
                        break;
                    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                        ss << "(Undefined) ";
                        break;
                    case GL_DEBUG_TYPE_PORTABILITY:
                        ss << "(Portability) ";
                        break;
                    case GL_DEBUG_TYPE_PERFORMANCE:
                        ss << "(Performance) ";
                        break;
                    case GL_DEBUG_TYPE_MARKER:
                        ss << "(Marker) ";
                        break;
                    case GL_DEBUG_TYPE_PUSH_GROUP:
                        ss << "(Push Group) ";
                        break;
                    case GL_DEBUG_TYPE_POP_GROUP:
                        ss << "(Pop Group) ";
                        break;
                    case GL_DEBUG_TYPE_OTHER:
                        ss << "(Other) ";
                        break;
                    case GL_DONT_CARE:
                    default:
                        ss << "(Unknown) ";
                        break;
                }

                switch (severity)
                {
                case GL_DEBUG_SEVERITY_NOTIFICATION:
                    ss << " notify : ";
                    break;
                case GL_DEBUG_SEVERITY_LOW:
                    ss << " info : ";
                    break;
                case GL_DEBUG_SEVERITY_MEDIUM:
                    ss << " medium : ";
                    break;
                case GL_DEBUG_SEVERITY_HIGH:
                    ss << " IMPORTANT : ";
                    break;
                case GL_DONT_CARE:
                default:
                    ss << " unknown : ";
                    break;
                }

                // possibly use userid to block messages
                std::cout << ss.str();
            }, nullptr);
        }
#endif
    }

    window::window(wrap_g &__graphics, GLint width, GLint height, const GLchar *title, const window &win, bool fullscreen) noexcept
        : __graphics(__graphics), m_width(width), m_height(height), m_title(title)
    {
        // check whether the shared context window is empty
        if (win.win() == nullptr)
        {
            __graphics.out() << "[wrap_g] Error: Shared resources context is empty.\n";
            return;
        }
        
        // create the window with the parameters
        m_win = glfwCreateWindow(width, height, title, fullscreen ? glfwGetPrimaryMonitor() : nullptr, win.m_win);
        
        // check whether the window was actually created
        if (m_win == nullptr)
        {
            __graphics.out() << "[wrap_g] Error: Failed to create window.\n";
            return;
        }
        // make it the current context in this thread
        glfwMakeContextCurrent(m_win);

        // make sure glad is loaded properly
        if (!check_glad())
        {
            __graphics.out() << "[wrap_g] Error: Failed to initialize glad.\n";
            return;
        }

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Created window.\n";
#endif
#if WRAP_G_USE_NEW_OPENGL_DEBUG_MESSAGE_CONTROL
        // check whether debug context was activated
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            // enable the debug output
            glEnable(GL_DEBUG_OUTPUT);

            // make the output synchron6ous
            // so the output commands are called immediately
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

            // allow all debug messages
            // this function can be used to control debug outputs
            // for sources, types, severity
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
            
            // actual debug message output function
            glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei llength, const GLchar* message, const void *user_param){
                std::stringstream ss;

                switch (source)
                {
                case GL_DEBUG_SOURCE_API:
                    ss << "[opengl api] ";
                    break;
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                    ss << "[window system] ";
                    break;
                case GL_DEBUG_SOURCE_SHADER_COMPILER:
                    ss << "[shader compiler] ";
                    break;
                case GL_DEBUG_SOURCE_THIRD_PARTY:
                    ss << "[third party] ";
                    break;
                case GL_DEBUG_SOURCE_APPLICATION:
                    ss << "[application] ";
                    break;
                case GL_DEBUG_SOURCE_OTHER:
                    ss << "[other] ";
                    break;
                case GL_DONT_CARE:
                default:
                    ss << "[unknown] ";
                    break;
                }
                
                switch (type)
                {
                    case GL_DEBUG_TYPE_ERROR:
                        ss << "(Error) ";
                        break;
                    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                        ss << "(Deprecated) ";
                        break;
                    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                        ss << "(Undefined) ";
                        break;
                    case GL_DEBUG_TYPE_PORTABILITY:
                        ss << "(Portability) ";
                        break;
                    case GL_DEBUG_TYPE_PERFORMANCE:
                        ss << "(Performance) ";
                        break;
                    case GL_DEBUG_TYPE_MARKER:
                        ss << "(Marker) ";
                        break;
                    case GL_DEBUG_TYPE_PUSH_GROUP:
                        ss << "(Push Group) ";
                        break;
                    case GL_DEBUG_TYPE_POP_GROUP:
                        ss << "(Pop Group) ";
                        break;
                    case GL_DEBUG_TYPE_OTHER:
                        ss << "(Other) ";
                        break;
                    case GL_DONT_CARE:
                    default:
                        ss << "(Unknown) ";
                        break;
                }

                switch (severity)
                {
                case GL_DEBUG_SEVERITY_NOTIFICATION:
                    ss << " notify : ";
                    break;
                case GL_DEBUG_SEVERITY_LOW:
                    ss << " info : ";
                    break;
                case GL_DEBUG_SEVERITY_MEDIUM:
                    ss << " medium : ";
                    break;
                case GL_DEBUG_SEVERITY_HIGH:
                    ss << " IMPORTANT : ";
                    break;
                case GL_DONT_CARE:
                default:
                    ss << " unknown : ";
                    break;
                }

                // possibly use userid to block messages
                std::cout << ss.str();
            }, nullptr);
        }
#endif
    }

    [[nodiscard]] inline std::pair<double, double> window::get_cursor_position() const noexcept
    {
        // get the current position of the cursor
        // just retreives the values from the inner glfw
        // function
        std::pair<double, double> pos;
        glfwGetCursorPos(m_win, &pos.first, &pos.second);
        return pos;
    }

    template <typename Fn>
    requires FramebufferSizeCallback<Fn>
    void window::set_framebuffer_size_callback(Fn fn) noexcept
    {
        // set the framebuffer size callback
        // just forwards to the inner glfw function
        glfwSetFramebufferSizeCallback(m_win, fn);
    }

    template <typename Fn>
    requires KeyCallback<Fn>
    void window::set_key_callback(Fn fn) noexcept
    {
        // set the key callback
        // just forwards to inner glfw function
        glfwSetKeyCallback(m_win, fn);
    }

    template <typename Fn>
    requires CursorPositionCallback<Fn>
    void window::set_cursor_position_callback(Fn fn) noexcept
    {
        // set the cursor position callback
        // also forwards to inner glfw function
        glfwSetCursorPosCallback(m_win, fn);
    }

    template <typename Fn>
    requires MouseButtonCallback<Fn>
    void window::set_mouse_button_callback(Fn fn) noexcept
    {
        // set the mouse button callback
        // also forwards to inner glfw function
        glfwSetMouseButtonCallback(m_win, fn);
    }

    void window::set_should_close(bool close) noexcept
    {
        // set the window should close parameter
        // does not neccessarily imply that the window
        // will immediately close
        glfwSetWindowShouldClose(m_win, close);
    }

    void window::set_current_context() noexcept
    {
        // set the window as the currrent context for this thread
        glfwMakeContextCurrent(m_win);
    }

    void window::swap_buffers() noexcept
    {
        // swap the displayed buffer with the hidden draw buffer
        // call once all draw calls are done to display the results 
        // in the window
        glfwSwapBuffers(m_win);
    }

    void window::set_buffer_swap_interval(int interval) noexcept
    {
        // swap the window buffer at the time
        // set to 0 for vsync
        glfwSwapInterval(interval);
    }

    vertex_array_object window::create_vao() noexcept
    {
        // create a vertex array object to store vertex information
        return vertex_array_object(__graphics);
    }

    program window::create_program() noexcept
    {
        // create a program to store shader information
        return program(__graphics);
    }

    texture window::create_texture(GLenum target) noexcept
    {
        // create a texture to store an image
        // and possibly reference as a sampler in one
        // of the shader programs
        return texture(__graphics, target);
    }

    ////
    // vertex array object

    vertex_array_object::vertex_array_object(wrap_g &__graphics) noexcept
        : __graphics(__graphics)
    {
        // create the vertex array
        // and get an id
        glCreateVertexArrays(1, &m_id);

        // make sure the id is valid
        if (m_id == 0)
        {
            __graphics.out() << "[wrap_g] Error: Failed to create VAO.\n";
            return;
        }

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Created VAO #" << m_id << ".\n";
#endif
    }

    vertex_array_object::~vertex_array_object() noexcept
    {
        // delete all the array buffers
        for (const auto &[binding_index, buffer] : m_array_buffers)
        {
            glDeleteBuffers(1, &buffer.buffer_id);

#if WRAP_G_DEBUG
            __graphics.out() << "[wrap_g] Debug: Deleted VAO #" << m_id << " array buffer #" << buffer.buffer_id << ".\n";
#endif
        }

        // delete the element array buffer
        glDeleteBuffers(1, &m_element_buffer_id);

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Deleted VAO #" << m_id << " element buffer #" << m_element_buffer_id << ".\n";
#endif

        // delete the vertex array object
        glDeleteVertexArrays(1, &m_id);

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Deleted VAO #" << m_id << ".\n";
#endif
    }

    template <typename Wrapper>
    void vertex_array_object::create_array_buffer(GLuint binding_index, GLsizeiptr buffer_size, Wrapper *data, GLbitfield flags, GLintptr offset) noexcept
    {
        GLuint buffer_id = 0;
    
        // create the array buffer
        glCreateBuffers(1, &buffer_id);
        
        // make sure the id is valid
        if (buffer_id == 0)
        {
            __graphics.out() << "[wrap_g] Error: Failed to create VAO #" << m_id << " array buffer.\n";
            return;
        }

        // assign the pointer to the data of the buffer
        glNamedBufferStorage(buffer_id, buffer_size, data, flags);
        
        // assign the buffer a binding index
        glVertexArrayVertexBuffer(m_id, binding_index, buffer_id, offset, sizeof(Wrapper));

        // add the binding index to the array buffer index list
        m_array_buffers.insert_or_assign(binding_index, array_buffer{buffer_id, sizeof(Wrapper)});

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Created VAO #" << m_id << " array buffer #" << buffer_id << " and is bound to binding index: " << binding_index << ".\n";
#endif
    }

    template <typename Wrapper>
    void vertex_array_object::create_array_buffer(GLuint binding_index, GLsizeiptr buffer_size, Wrapper *data, GLsizei stride, GLbitfield flags, GLintptr offset) noexcept
    {
        GLuint buffer_id = 0;

        // create the array buffer
        glCreateBuffers(1, &buffer_id);
        
        // make sure the id is valid
        if (buffer_id == 0)
        {
            __graphics.out() << "[wrap_g] Error: Failed to create VAO #" << m_id << " array buffer.\n";
            return;
        }
        
        // assign the pointer to the data of the buffer with additional flags
        glNamedBufferStorage(buffer_id, buffer_size, data, flags);
        
        // assign the buffer a binding index
        glVertexArrayVertexBuffer(m_id, binding_index, buffer_id, offset, stride);

        // add the binding index to the array buffer index list
        m_array_buffers.insert_or_assign(binding_index, array_buffer{buffer_id, sizeof(Wrapper)});

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Created VAO #" << m_id << " array buffer #" << buffer_id << " and is bound to binding index: " << binding_index << ".\n";
#endif
    }

    template <typename Wrapper>
    void vertex_array_object::create_element_buffer(GLsizeiptr buffer_size, Wrapper *data, GLbitfield flags) noexcept
    {
        GLuint buffer_id = 0;
        
        // create the array buffer
        glCreateBuffers(1, &buffer_id);
        
        // make sure the id is valid
        if (buffer_id == 0)
        {
            __graphics.out() << "[wrap_g] Error: Failed to create VAO #" << m_id << " element buffer.\n";
            return;
        }

        // assign the pointer to the data of the buffer with additional flags
        glNamedBufferStorage(buffer_id, buffer_size, data, flags);
        
        // set the element array buffer
        glVertexArrayElementBuffer(m_id, buffer_id);

        // set the element buffer id
        m_element_buffer_id = buffer_id;

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Created VAO #" << m_id << " element buffer #" << buffer_id << ".\n";
#endif
    }

    void vertex_array_object::define_attrib(GLuint binding_index, GLuint attrib_index, GLint count, GLenum data_type, bool normalised, GLuint relative_offset) noexcept
    {
        // enable the attribute index
        glEnableVertexArrayAttrib(m_id, attrib_index);
        
        // check if data is one of the int types
        if (utils::one_of<GLenum>(data_type, {GL_BYTE, GL_SHORT, GL_INT, GL_FIXED, GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_UNSIGNED_INT, GL_INT_2_10_10_10_REV, GL_UNSIGNED_INT_10_10_10_2, GL_UNSIGNED_INT_10F_11F_11F_REV}))
        {
            // declare the data contained within the attribute
            glVertexArrayAttribIFormat(m_id, attrib_index, count, data_type, relative_offset);
        }
        
        // check if data is double
        if (data_type == GL_DOUBLE)
        {
            // declare the data contained within the attribute
            glVertexArrayAttribLFormat(m_id, attrib_index, count, data_type, relative_offset);
        }
        
        // declare the data contained within the attribute
        glVertexArrayAttribFormat(m_id, attrib_index, count, data_type, normalised, relative_offset);
        
        // identify which buffer has a binding index that contains this attribute
        glVertexArrayAttribBinding(m_id, attrib_index, binding_index);
        
#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Defined attributes for VAO #" << m_id << ", buffer binding index: " << binding_index << ", attribute index: " << attrib_index << ".\n";
#endif
    }

    void vertex_array_object::bind() const noexcept
    {
        // bind this vertex array to the current context
        glBindVertexArray(m_id);
    }

    ////
    // program

    program::program(wrap_g &__graphics) noexcept
        : __graphics(__graphics)
    {
        // create an opengl shader program
        m_id = glCreateProgram();

        // check if the shader is valid
        if (m_id == 0)
        {
            __graphics.out() << "[wrap_g] Error: Failed to create program #" << m_id << ".\n";
            return;
        }

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Created program #" << m_id << ".\n";
#endif
    }

    program::~program() noexcept
    {
        for (const auto &[shader_type, ids] : m_shaders)
        {
            for (const auto &id : ids)
            {
                // delete each sub shader
                glDeleteShader(id);

#if WRAP_G_DEBUG
                __graphics.out() << "[wrap_g] Debug: Deleted program #" << m_id << " shader #" << id << ".\n";
#endif
            }
        }

        // delete the shader program
        glDeleteProgram(m_id);

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Deleted program #" << m_id << ".\n";
#endif
    }

    template<bool paths, bool async, typename String>
    requires utils::Stringable<String>
    [[nodiscard]] __wrap_g_prog_quick_ret_t<paths && async> program::quick(const std::unordered_map<GLenum, std::vector<String>> &shaders) noexcept
    {
        bool success = true;
        // use the provided shader locations
        for (const auto &[shader_type, info_arr] : shaders)
        {
            // sync vs async per type vs full async
#if WRAP_G_BACKGROUND_RESOURCE_LOAD
            for (const auto &info : info_arr)
            {
                if constexpr (paths)
                {
                    if constexpr(async) {
                        
                    }
                    else {
                        // read the file data
                        std::string code = utils::read_file_sync(((std::string_view)info).data());

                        // compile the sub shader
                        // and add it to the program
                        success = success && create_shader(shader_type, code.c_str());
                    }
                }
                else
                {
                    // compile the sub shader
                    // and add it to the program
                    success = success && create_shader(shader_type, ((std::string_view)info).data());
                }
            }
#else
            for (const auto &info : info_arr)
            {
                if constexpr (paths)
                {
                    if constexpr(async) {
                        
                    }
                    else {
                        // read the file data
                        std::string code = utils::read_file_sync(((std::string_view)info).data());

                        // compile the sub shader
                        // and add it to the program
                        success = success && create_shader(shader_type, code.c_str());
                    }
                }
                else
                {
                    // compile the sub shader
                    // and add it to the program
                    success = success && create_shader(shader_type, ((std::string_view)info).data());
                }
            }
#endif
        }
        
        // link all of the currently added shaders
        return success && link_shaders();
    }

    [[nodiscard]] bool program::create_shader(GLenum shader_type, const char *code) noexcept
    {
        // create a sub shader of the desired type ex: fragment or vertex shader
        GLuint shader_id = glCreateShader(shader_type);

        // checking if the shader is valid
        if (shader_id == 0)
        {
            __graphics.out() << "[wrap_g] Error: Failed to create program #" << m_id << " shader.\n";
            return false;
        }

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Created program #" << m_id << " shader #" << shader_id << ".\n";
#endif

        // add the sub shader source to opengl current context
        glShaderSource(shader_id, 1, &code, NULL);

        // compile the sub shader
        glCompileShader(shader_id);

        // check if the shader is compiled
        int success = 0;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
        
        // if not compiled
        if (!success)
        {   
            // get the error log from opengl
            constexpr size_t size = 512;
            char info[size];
            glGetShaderInfoLog(shader_id, size, NULL, info);
            __graphics.out() << "[wrap_g] Error: Failed to compile program #" << m_id << " shader #" << shader_id << ". " << info << "\n";

            // delete the shader
            glDeleteShader(shader_id);

#if WRAP_G_DEBUG
            __graphics.out() << "[wrap_g] Debug: Deleting program #" << m_id << " shader #" << shader_id << ".\n ";
#endif
            return false;
        }

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Compiled program #" << m_id << " shader #" << shader_id << ".\n";
#endif

        // attach the shader to the current program
#if WRAP_G_BACKGROUND_RESOURCE_LOAD
        std::lock_guard<std::mutex> lock_prog(_thread_guard);
#endif
        glAttachShader(m_id, shader_id);

        return true;
    }

    [[nodiscard]] bool program::link_shaders() noexcept
    {
        // link all the currently attached shaders to the current program
        glLinkProgram(m_id);

        // check if linking was successful
        int success = 0;
        glGetProgramiv(m_id, GL_LINK_STATUS, &success);

        // if not linked
        if (!success)
        {
            // get the error log from opengl
            constexpr size_t size = 512;
            char info[size];
            glGetProgramInfoLog(m_id, size, NULL, info);
            __graphics.out() << "[wrap_g] Error: Failed to link program. " << info << "\n";
            return false;
        }

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Linked program #" << m_id << ".\n";
#endif
    
        return true;
    }

    void program::use() const noexcept
    {
        // use the current program in the current context
        glUseProgram(m_id);
    }

    int program::uniform_location(const char *name) const noexcept
    {
        // get the location of a uniform in a shader using its name
        return glGetUniformLocation(m_id, name);
    }

    // TODO: check if array version is better
    template <typename... Ts>
    requires((utils::Stringable<Ts> && ...))
    std::vector<int> program::uniform_locations(Ts &&...names) const noexcept
    {
        // gets the locations of all the uniforms in the
        // order they were provided in the arguments
        // this function can be typically used with an
        // enum to keep constants readable
        std::vector<int> uniforms;

        for (std::string_view name : std::initializer_list{names...})
            uniforms.push_back(glGetUniformLocation(m_id, name.data()));

        return uniforms;
    }

    // template <typename... Ts>
    // requires((utils::Stringable<Ts> && ...))
    // std::array<int, sizeof...(Ts)> program::uniform_locations(Ts &&...names) const noexcept
    // {
    //     // gets the locations of all the uniforms in the
    //     // order they were provided in the arguments
    //     // this function can be typically used with an
    //     // enum to keep constants readable
    //     std::array<int, sizeof...(Ts)> uniforms;
    //     int i = 0;

    //     for (std::string_view name : std::initializer_list{names...})
    //     {
    //         uniforms[i++] = glGetUniformLocation(m_id, name.data());
    //     }

    //     return uniforms;
    // }

    template <typename... Ts>
    requires(std::is_integral_v<Ts> &&...) || (std::is_floating_point_v<Ts> && ...) void program::set_uniform(int loc, const Ts &...vals) noexcept
    {
        // count the number of values provided in compile time
        constexpr size_t count = sizeof...(Ts);
        
        // set the uniform according to the data type and the number of values provided
        if constexpr (count == 1)
        {
            if constexpr ((std::is_same_v<Ts, unsigned int> && ...))
                glProgramUniform1ui(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, int> && ...) || (std::is_same_v<Ts, bool> && ...))
                glProgramUniform1i(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, float> && ...))
                glProgramUniform1f(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, double> && ...))
                glProgramUniform1d(m_id, loc, vals...);
        }

        if constexpr (count == 2)
        {
            if constexpr ((std::is_same_v<Ts, unsigned int> && ...))
                glProgramUniform2ui(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, int> && ...) || (std::is_same_v<Ts, bool> && ...))
                glProgramUniform2i(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, float> && ...))
                glProgramUniform2f(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, double> && ...))
                glProgramUniform2d(m_id, loc, vals...);
        }

        if constexpr (count == 3)
        {
            if constexpr ((std::is_same_v<Ts, unsigned int> && ...))
                glProgramUniform3ui(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, int> && ...) || (std::is_same_v<Ts, bool> && ...))
                glProgramUniform3i(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, float> && ...))
                glProgramUniform3f(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, double> && ...))
                glProgramUniform3d(m_id, loc, vals...);
        }

        if constexpr (count == 4)
        {
            if constexpr ((std::is_same_v<Ts, unsigned int> && ...))
                glProgramUniform4ui(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, int> && ...) || (std::is_same_v<Ts, bool> && ...))
                glProgramUniform4i(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, float> && ...))
                glProgramUniform4f(m_id, loc, vals...);
            if constexpr ((std::is_same_v<Ts, double> && ...))
                glProgramUniform4d(m_id, loc, vals...);
        }
    }

    template <size_t vec_size, typename T>
    void program::set_uniform_vec(int loc, T *val, size_t count) noexcept
    {
        // set the uniform vectors according to the vector size and data type
        if constexpr (vec_size == 1)
        {
            if constexpr (std::is_same_v<std::remove_cv_t<T>, unsigned int>)
                glProgramUniform1uiv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, int> || std::is_same_v<std::remove_cv_t<T>, bool>)
                glProgramUniform1iv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, float>)
                glProgramUniform1fv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
                glProgramUniform1dv(m_id, loc, count, val);
        }

        if constexpr (vec_size == 2)
        {
            if constexpr (std::is_same_v<std::remove_cv_t<T>, unsigned int>)
                glProgramUniform2uiv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, int> || std::is_same_v<std::remove_cv_t<T>, bool>)
                glProgramUniform2iv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, float>)
                glProgramUniform2fv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
                glProgramUniform2dv(m_id, loc, count, val);
        }

        if constexpr (vec_size == 3)
        {
            if constexpr (std::is_same_v<std::remove_cv_t<T>, unsigned int>)
                glProgramUniform3uiv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, int> || std::is_same_v<std::remove_cv_t<T>, bool>)
                glProgramUniform3iv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, float>)
                glProgramUniform3fv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
                glProgramUniform3dv(m_id, loc, count, val);
        }

        if constexpr (vec_size == 4)
        {
            if constexpr (std::is_same_v<std::remove_cv_t<T>, unsigned int>)
                glProgramUniform4uiv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, int> || std::is_same_v<std::remove_cv_t<T>, bool>)
                glProgramUniform4iv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, float>)
                glProgramUniform4fv(m_id, loc, count, val);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
                glProgramUniform4dv(m_id, loc, count, val);
        }
    }

    template <size_t cols, size_t rows, typename T>
    void program::set_uniform_mat(int loc, T *val, size_t count, bool transpose) noexcept
    {
        // matrix for doubles
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
        {
            // set the matrix uniforms based on the matrix dimensions
            if constexpr (rows == 2)
            {
                if constexpr (cols == 2)
                    glProgramUniformMatrix2dv(m_id, loc, count, transpose, val);
                if constexpr (cols == 3)
                    glProgramUniformMatrix2x3dv(m_id, loc, count, transpose, val);
                if constexpr (cols == 4)
                    glProgramUniformMatrix2x4dv(m_id, loc, count, transpose, val);
            }

            if constexpr (rows == 3)
            {
                if constexpr (cols == 2)
                    glProgramUniformMatrix3x2dv(m_id, loc, count, transpose, val);
                if constexpr (cols == 3)
                    glProgramUniformMatrix3dv(m_id, loc, count, transpose, val);
                if constexpr (cols == 4)
                    glProgramUniformMatrix3x4dv(m_id, loc, count, transpose, val);
            }

            if constexpr (rows == 4)
            {
                if constexpr (cols == 2)
                    glProgramUniformMatrix4x2dv(m_id, loc, count, transpose, val);
                if constexpr (cols == 3)
                    glProgramUniformMatrix4x3dv(m_id, loc, count, transpose, val);
                if constexpr (cols == 4)
                    glProgramUniformMatrix4dv(m_id, loc, count, transpose, val);
            }
        }
        // matrix for floats
        if constexpr (std::is_same_v<std::remove_cv_t<T>, float>)
        {
            // set the matrix uniforms based on the matrix dimensions
            if constexpr (rows == 2)
            {
                if constexpr (cols == 2)
                    glProgramUniformMatrix2fv(m_id, loc, count, transpose, val);
                if constexpr (cols == 3)
                    glProgramUniformMatrix2x3fv(m_id, loc, count, transpose, val);
                if constexpr (cols == 4)
                    glProgramUniformMatrix2x4fv(m_id, loc, count, transpose, val);
            }

            if constexpr (rows == 3)
            {
                if constexpr (cols == 2)
                    glProgramUniformMatrix3x2fv(m_id, loc, count, transpose, val);
                if constexpr (cols == 3)
                    glProgramUniformMatrix3fv(m_id, loc, count, transpose, val);
                if constexpr (cols == 4)
                    glProgramUniformMatrix3x4fv(m_id, loc, count, transpose, val);
            }

            if constexpr (rows == 4)
            {
                if constexpr (cols == 2)
                    glProgramUniformMatrix4x2fv(m_id, loc, count, transpose, val);
                if constexpr (cols == 3)
                    glProgramUniformMatrix4x3fv(m_id, loc, count, transpose, val);
                if constexpr (cols == 4)
                    glProgramUniformMatrix4fv(m_id, loc, count, transpose, val);
            }
        }
    }

    ////
    // texture

    texture::texture(wrap_g &__graphics, GLenum target) noexcept
        : __graphics(__graphics), m_target(target)
    {
        // create the texture
        glCreateTextures(target, 1, &m_id);
        
        // check if the id is valid
        if (m_id == 0)
        {
            __graphics.out() << "[wrap_g] Error: Failed to create texture.\n";
            return;
        }

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Created texture #" << m_id << ".\n";
#endif
    }

    texture::~texture() noexcept
    {
        // delete the texture
        glDeleteTextures(1, &m_id);

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Deleted texture #" << m_id << ".\n";
#endif
    }

    void texture::recreate() noexcept
    {
        glDeleteTextures(1, &m_id);
        
#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Deleted texture #" << m_id << ".\n";
#endif

        // create the texture
        glCreateTextures(m_target, 1, &m_id);
        
        // check if the id is valid
        if (m_id == 0)
        {
            __graphics.out() << "[wrap_g] Error: Failed to re-create texture.\n";
            return;
        }

#if WRAP_G_DEBUG
        __graphics.out() << "[wrap_g] Debug: Re-Created texture #" << m_id << ".\n";
#endif
    }

    void texture::bind_unit(GLuint texture_unit) const noexcept
    {
        // bind the texture to the current context
        glBindTextureUnit(texture_unit, m_id);
    }

    template <typename T>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
    void texture::set_param(GLenum param, const T &val) noexcept
    {
        // set the texture parameters of integer types
        if constexpr (std::is_integral_v<T>)
        {
            // set the texture parameter
            glTextureParameteri(m_id, param, val);
        }

        // set the texture parameters of floating point types
        if constexpr (std::is_floating_point_v<T>)
        {
            // set the texture parameter
            glTextureParameterf(m_id, param, val);
        }
    }

    template <typename T, typename ForceType>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
    void texture::set_param_vec(GLenum param, const T *arr) noexcept
    {
        // set the texture parameter for unsigned integers
        // properly as integer values
        if constexpr (std::is_same_v<std::remove_cv_t<T>, unsigned int>)
            glTextureParameterIuiv(m_id, param, arr);

        // set the texture parameter for integers
        if constexpr (std::is_same_v<std::remove_cv_t<T>, int>)
        {
            if constexpr (std::is_same_v<std::remove_cv_t<T>, ForceType>)
            {
                // force opengl to store the value as an integer
                glTextureParameterIiv(m_id, param, arr);
            }
            else
            {
                // the integer is stored as a float
                glTextureParameteriv(m_id, param, arr);
            }
        }

        // set the texture parameter for floats
        if constexpr (std::is_floating_point_v<T>)
        {
            glTextureParameterfv(m_id, param, arr);
        }
    }

    void texture::define_texture2d(size_t levels, GLenum internal_format, size_t width, size_t height) noexcept
    {
        // create immovable storage for a 2d texture
        glTextureStorage2D(m_id, levels, internal_format, width, height);
    }

    void texture::sub_image2d(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) noexcept
    {
        // fill the space created in the storage
        glTextureSubImage2D(m_id, level, xoffset, yoffset, width, height, format, type, pixels);
    }

    void texture::gen_mipmap() noexcept
    {
        // generate mipmaps
        glGenerateTextureMipmap(m_id);
    }

} // namespace wrap_g

#endif