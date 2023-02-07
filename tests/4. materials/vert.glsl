#version 450 core

layout (location = 0) in vec3 ipos;
layout (location = 1) in vec3 inormals;
layout (location = 2) in vec2 itex_coord;

out vec3 frag_pos;
out vec3 normals;
out vec2 tex_coord;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normal_mat;

void main()
{
    tex_coord = itex_coord;
    normals = normal_mat * inormals;
    frag_pos = vec3(model * vec4(ipos, 1.0));
    gl_Position = proj * view * model * vec4(ipos.xyz, 1.0);
}