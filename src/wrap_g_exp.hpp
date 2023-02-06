#ifndef WRAP_G_EXP_HPP
#define WRAP_G_EXP_HPP

// glm
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// stl
#include <unordered_map>

// local
#include "wrap_g.hpp"

namespace wrap_g
{

////////
// concepts
////////

////////
// Declarations
////////

////
// Rendering
////

////
// Absolute Basic

template<typename T = float>
requires (std::is_same_v<T, double> || std::is_same_v<T, float>)
struct observer
{
    glm::mat4 _proj {1.};
    glm::mat4 _view {1.};
};

template<typename T = float>
requires (std::is_same_v<T, double> || std::is_same_v<T, float>)
class object
{
public:
    glm::mat4 _model {1.};
};

class gl_object
{
public:
    window& _context;
    vertex_array_object _vao;
    program _prog;

    gl_object(window& context) noexcept : _context(context), _vao(context.create_vao()), _prog(context.create_program()) {}
};

////
// Basic Shapes

class rect
{
public:
    gl_object _base_gl;
    object<> _base;
    std::unordered_map<std::string_view, int> m_uniforms_locs;
    size_t m_indices_size;

public:
    rect(window& context) : _base_gl(context)
    {
        constexpr auto verts = utils::gen_rect_verts<3>(glm::vec3{-0.5f, -0.5f, 0.0f}, glm::vec3{0.5f, 0.5f, 0.0f});
        constexpr auto tex_coords = utils::gen_rect_verts<2>(glm::vec2{0.0f}, glm::vec2{1.0f});
        constexpr auto indices = utils::gen_rect_indices();

        _base_gl._vao.define_attrib(0, 0, 3, GL_FLOAT);
        _base_gl._vao.define_attrib(1, 1, 2, GL_FLOAT);

        _base_gl._vao.create_array_buffer(0, verts.size() * sizeof(glm::vec3), verts.data(), GL_MAP_READ_BIT);
        _base_gl._vao.create_array_buffer(1, tex_coords.size() * sizeof(glm::vec2), tex_coords.data(), GL_MAP_READ_BIT);

        m_indices_size = indices.size();
        _base_gl._vao.create_element_buffer(m_indices_size * sizeof(glm::uvec3), indices.data(), GL_MAP_READ_BIT);
    }

    template<typename String>
    requires utils::Stringable<String>
    bool prog_quick(const std::unordered_map<GLenum, std::vector<String>> &shaders) noexcept
    {
        return _base_gl._prog.quick(std::forward<const std::unordered_map<GLenum, std::vector<String>> &>(shaders));
    }

    template<typename ... Uniforms>
    requires (utils::Stringable<Uniforms> && ...)
    void save_uniforms(Uniforms&& ... uniforms_names) noexcept
    {
        for (const auto& uniform_name : std::initializer_list{uniforms_names...}) {
            int uniform_loc = _base_gl._prog.uniform_location(uniform_name);
            if (uniform_loc == -1)
                continue;

            m_uniforms_locs.insert({{uniform_name, uniform_loc}});
        }
    }

    template<typename String>
    requires utils::Stringable<String>
    void save_uniforms(std::initializer_list<String> uniforms_names) noexcept
    {
        for (const auto& uniform_name : uniforms_names) {
            int uniform_loc = _base_gl._prog.uniform_location(uniform_name);
            if (uniform_loc == -1)
                continue;

            m_uniforms_locs.insert({{uniform_name, uniform_loc}});
        }
    }

    template<typename ... Ts>
    requires(std::is_integral_v<Ts> &&...) || (std::is_floating_point_v<Ts> && ...)
    void set_uniform(std::string_view name, const Ts&... vals) noexcept
    {
        auto it = m_uniforms_locs.find(name);
        if (it == m_uniforms_locs.end())
        {
            int loc = _base_gl._prog.uniform_location(name);
            if (loc == -1)
                return;
            
            m_uniforms_locs.insert({{name, loc}});
            it = m_uniforms_locs.find(name);
        }

        _base_gl._prog.set_uniform(it->second, vals...);
    }

    template<size_t vec_size, typename T = float>
    requires std::is_floating_point_v<T>
    void set_uniform_vec(std::string_view name, T *val, size_t count = 1) noexcept
    {
        auto it = m_uniforms_locs.find(name);
        if (it == m_uniforms_locs.end())
        {
            int loc = _base_gl._prog.uniform_location(name);
            if (loc == -1)
                return;
            
            m_uniforms_locs.insert({{name, loc}});
            it = m_uniforms_locs.find(name);
        }

        _base_gl._prog.set_uniform_vec<vec_size, T>(it->second, val, count);
    }

    template<size_t rows, size_t cols = rows, typename T = float>
    requires std::is_floating_point_v<T>
    void set_uniform_mat(std::string_view name, T *val, size_t count = 1, bool transpose = false) noexcept
    {
        auto it = m_uniforms_locs.find(name);
        if (it == m_uniforms_locs.end())
        {
            int loc = _base_gl._prog.uniform_location(name);
            if (loc == -1)
                return;
            
            m_uniforms_locs.insert({{name, loc}});
            it = m_uniforms_locs.find(name);
        }

        _base_gl._prog.set_uniform_mat<rows, cols, T>(it->second, val, count, transpose);
    }

    void render() const noexcept
    {
        _base_gl._vao.bind();
        _base_gl._prog.use();

        glDrawElements(GL_TRIANGLES, m_indices_size * sizeof(glm::uvec3) / sizeof(unsigned int), GL_UNSIGNED_INT, nullptr);
    }
};

class cube
{
public:
    gl_object _base_gl;
    object<> _base;
    std::unordered_map<std::string_view, int> m_uniforms_locs;
    size_t m_verts_size;

public:
    cube(window& context) : _base_gl(context)
    {
        constexpr auto verts = utils::gen_cube_verts(glm::vec3{-0.5f}, glm::vec3{0.5f});
        constexpr auto tex_coords = utils::gen_cube_texcoords();

        _base_gl._vao.define_attrib(0, 0, 3, GL_FLOAT);
        _base_gl._vao.define_attrib(1, 1, 2, GL_FLOAT);

        _base_gl._vao.create_array_buffer(0, verts.size() * sizeof(glm::vec3), verts.data(), GL_MAP_READ_BIT);
        m_verts_size = verts.size();
        _base_gl._vao.create_array_buffer(1, tex_coords.size() * sizeof(glm::vec2), tex_coords.data(), GL_MAP_READ_BIT);
    }

    template<typename String>
    requires utils::Stringable<String>
    bool prog_quick(const std::unordered_map<GLenum, std::vector<String>> &shaders) noexcept
    {
        return _base_gl._prog.quick(std::forward<const std::unordered_map<GLenum, std::vector<String>> &>(shaders));
    }

    template<typename ... Uniforms>
    requires (utils::Stringable<Uniforms> && ...)
    void save_uniforms(Uniforms&& ... uniforms_names) noexcept
    {
        for (const auto& uniform_name : std::initializer_list{uniforms_names...}) {
            int uniform_loc = _base_gl._prog.uniform_location(uniform_name);
            if (uniform_loc == -1)
                continue;

            m_uniforms_locs.insert({{uniform_name, uniform_loc}});
        }
    }

    template<typename String>
    requires utils::Stringable<String>
    void save_uniforms(std::initializer_list<String> uniforms_names) noexcept
    {
        for (const auto& uniform_name : uniforms_names) {
            int uniform_loc = _base_gl._prog.uniform_location(uniform_name);
            if (uniform_loc == -1)
                continue;

            m_uniforms_locs.insert({{uniform_name, uniform_loc}});
        }
    }

    template<typename ... Ts>
    requires(std::is_integral_v<Ts> &&...) || (std::is_floating_point_v<Ts> && ...)
    void set_uniform(std::string_view name, const Ts&... vals) noexcept
    {
        auto it = m_uniforms_locs.find(name);
        if (it == m_uniforms_locs.end())
        {
            int loc = _base_gl._prog.uniform_location(name);
            if (loc == -1)
                return;
            
            m_uniforms_locs.insert({{name, loc}});
            it = m_uniforms_locs.find(name);
        }

        _base_gl._prog.set_uniform(it->second, vals...);
    }

    template<size_t vec_size, typename T = float>
    requires std::is_floating_point_v<T>
    void set_uniform_vec(std::string_view name, T *val, size_t count = 1) noexcept
    {
        auto it = m_uniforms_locs.find(name);
        if (it == m_uniforms_locs.end())
        {
            int loc = _base_gl._prog.uniform_location(name);
            if (loc == -1)
                return;
            
            m_uniforms_locs.insert({{name, loc}});
            it = m_uniforms_locs.find(name);
        }

        _base_gl._prog.set_uniform_vec<vec_size, T>(it->second, val, count);
    }

    template<size_t rows, size_t cols = rows, typename T = float>
    requires std::is_floating_point_v<T>
    void set_uniform_mat(std::string_view name, T *val, size_t count = 1, bool transpose = false) noexcept
    {
        auto it = m_uniforms_locs.find(name);
        if (it == m_uniforms_locs.end())
        {
            int loc = _base_gl._prog.uniform_location(name);
            if (loc == -1)
                return;
            
            m_uniforms_locs.insert({{name, loc}});
            it = m_uniforms_locs.find(name);
        }

        _base_gl._prog.set_uniform_mat<rows, cols, T>(it->second, val, count, transpose);
    }

    void render() const noexcept
    {
        _base_gl._vao.bind();
        _base_gl._prog.use();

        glDrawArrays(GL_TRIANGLES, 0, m_verts_size);
    }
};

} // namespace wrap_g

#include "wrap_g_exp_impl.hpp"

#endif 