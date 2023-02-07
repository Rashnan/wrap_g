#version 450 core

in vec2 texCoord;
out vec4 frag_col;

uniform vec4 col;

void main()
{
    frag_col = vec4(texCoord.xy, 0.0, 1.0);
}