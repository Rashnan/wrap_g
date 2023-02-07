#version 450 core

in vec2 texCoord;
out vec4 frag_col;

uniform vec4 col;
uniform vec4 light_col;

void main()
{
    float ambient_strength = 0.1;
    vec4 ambient = ambient_strength * light_col;

    vec4 res = ambient * col;
    frag_col = res;
}