#version 450 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex_coord;

out vec2 texCoord;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

void main()
{
    texCoord = tex_coord;
    gl_Position = proj * view * model * vec4(pos.xyz, 1.0);
}