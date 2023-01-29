#version 450 core

out vec4 frag_col;

uniform vec4 col;

void main()
{
    frag_col = col;
}