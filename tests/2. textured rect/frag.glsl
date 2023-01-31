#version 450 core

in vec2 texCoord;
out vec4 frag_col;

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform float tex_mix;

void main()
{
    vec4 tex1_col = texture(tex1, texCoord);
    vec4 tex2_col = texture(tex2, texCoord);
    frag_col = mix(tex1_col, tex2_col, tex_mix);
}