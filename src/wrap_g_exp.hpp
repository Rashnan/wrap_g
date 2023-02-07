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

struct observer
{
    glm::mat4 _proj {1.};
    glm::mat4 _view {1.};
};

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

struct rect
{
    gl_object _base_gl;
    object _base;
    size_t m_indices_size;

    rect(window& context) noexcept : _base_gl(context)
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

    void render() const noexcept
    {
        _base_gl._vao.bind();
        _base_gl._prog.use();

        glDrawElements(GL_TRIANGLES, m_indices_size * sizeof(glm::uvec3) / sizeof(unsigned int), GL_UNSIGNED_INT, nullptr);
    }
};

struct cube
{
    gl_object _base_gl;
    object _base;
    size_t m_verts_size;

    cube(window& context) noexcept : _base_gl(context)
    {
        constexpr auto verts = utils::gen_cube_verts(glm::vec3{-0.5f}, glm::vec3{0.5f});
        constexpr auto tex_coords = utils::gen_cube_texcoords();

        _base_gl._vao.define_attrib(0, 0, 3, GL_FLOAT);
        _base_gl._vao.define_attrib(1, 1, 2, GL_FLOAT);

        _base_gl._vao.create_array_buffer(0, verts.size() * sizeof(glm::vec3), verts.data(), GL_MAP_READ_BIT);
        m_verts_size = verts.size();
        _base_gl._vao.create_array_buffer(1, tex_coords.size() * sizeof(glm::vec2), tex_coords.data(), GL_MAP_READ_BIT);
    }

    void render() const noexcept
    {
        _base_gl._vao.bind();
        _base_gl._prog.use();

        glDrawArrays(GL_TRIANGLES, 0, m_verts_size);
    }
};

////
// Cameras

struct perspective_camera
{
    glm::mat4 m_proj;

    float m_start_fov;

    float m_fov;
    float m_aspect_ratio;
    float m_z_near;
    float m_z_far;

    perspective_camera(float start_fov, float aspect_ratio, float z_near, float z_far) noexcept
        : m_start_fov(start_fov), m_fov(start_fov), m_aspect_ratio(aspect_ratio), m_z_near(z_near), m_z_far(z_far)
    {
        m_proj = glm::perspective(glm::radians(m_fov), m_aspect_ratio, m_z_near, m_z_far);
    }

    void adjust_fov(float offset) noexcept
    {
        m_fov += offset;
        m_proj = glm::perspective(glm::radians(m_fov), m_aspect_ratio, m_z_near, m_z_far);
    }
    void reset_fov(float fov = 0.0f) noexcept
    {
        if (fov == 0.0f)
            fov = m_start_fov;
        
        m_start_fov = fov;
        m_fov = m_start_fov;
        m_proj = glm::perspective(glm::radians(m_fov), m_aspect_ratio, m_z_near, m_z_far);
    }
};

struct dynamic_camera
{
    glm::mat4 m_view;

    glm::vec3 m_start_pos;
    glm::vec3 m_start_look_at;

    glm::vec3 m_pos;

    glm::vec3 m_front;
    glm::vec3 m_right;
    glm::vec3 m_up;

    float pitch, yaw;

    dynamic_camera(const glm::vec3& start_pos, const glm::vec3& start_look_at, const glm::vec3& world_up = glm::vec3{0.0f, 1.0f, 0.0f}) noexcept
        : m_start_pos(start_pos), m_start_look_at(start_look_at), m_pos(start_pos)
    {
        m_view = glm::lookAt(m_pos, m_start_look_at, world_up);

        m_front = glm::normalize(m_start_look_at - m_pos);
        m_right = glm::normalize(glm::cross(m_front, world_up));
        m_up = glm::normalize(glm::cross(m_right, m_front));

        pitch = glm::degrees(glm::asin(m_front.y));
        yaw = glm::degrees(glm::asin(m_front.z / glm::cos(glm::radians(pitch))));
    }

    void move(const glm::vec3& offset) noexcept
    {
        m_pos += offset;
        m_view = glm::translate(m_view, -offset);
    }

    void rotate(const glm::vec2& cursor_offset, const glm::vec3& world_up = glm::vec3{0.0f, 1.0f, 0.0f}) noexcept
    {
        // using radial coords in 3d to calculate the new forward direction
        yaw += cursor_offset.x;
        pitch += cursor_offset.y;
        pitch = glm::clamp(pitch, -89.0f, 89.0f);

        m_front = glm::normalize(glm::vec3{
            glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
            glm::sin(glm::radians(pitch)),
            glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
        });

        m_right = glm::normalize(glm::cross(m_front, world_up));
        m_up = glm::normalize(glm::cross(m_right, m_front));

        // calculate view matrix
        m_view = glm::lookAt(m_pos, m_pos + m_front, world_up);
    }

    void reset(const glm::vec3& world_up = glm::vec3{0.0f, 1.0f, 0.0f}) noexcept
    {
        m_pos = m_start_pos;
        m_view = glm::lookAt(m_pos, m_start_look_at, world_up);
        
        m_front = glm::normalize(m_start_look_at - m_pos);
        m_right = glm::normalize(glm::cross(m_front, world_up));
        m_up = glm::normalize(glm::cross(m_right, m_front));

        pitch = glm::degrees(glm::asin(m_front.y));
        yaw = glm::degrees(glm::asin(m_front.z / glm::cos(glm::radians(pitch))));
    }
};

} // namespace wrap_g

#include "wrap_g_exp_impl.hpp"

#endif 