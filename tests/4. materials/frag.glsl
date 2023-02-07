#version 450 core

in vec3 frag_pos;
in vec3 normals;
in vec2 tex_coord;

out vec4 frag_col;

uniform vec4 col;
uniform vec4 light_col;
uniform vec3 light_pos;
uniform vec3 cam_pos;

void main()
{
    float ambient_strength = 0.1;
    vec4 ambient = ambient_strength * light_col;

    vec3 norm = normalize(normals);
    vec3 light_dir = normalize(light_pos - frag_pos);

    vec4 diffuse = max(dot(norm, light_dir), 0.0) * light_col;

    float specular_strength = 0.5;
    vec3 view_dir = normalize(cam_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), pow(2, 5));
    vec4 specular = specular_strength * spec * light_col;

    vec4 res = (ambient + diffuse + specular) * col;
    frag_col = res;
}