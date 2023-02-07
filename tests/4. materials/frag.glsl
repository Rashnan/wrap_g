#version 450 core

in vec3 frag_pos;
in vec3 normals;
in vec2 tex_coord;

out vec4 frag_col;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform Light light;
uniform vec3 cam_pos;

void main()
{
    vec3 ambient = light.ambient *  material.ambient;

    vec3 norm = normalize(normals);
    vec3 light_dir = normalize(light.position - frag_pos);

    vec3 diffuse = max(dot(norm, light_dir), 0.0) * light.diffuse * material.diffuse;

    vec3 view_dir = normalize(cam_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular = spec * light.specular * material.specular;

    vec3 res = (ambient + diffuse + specular);
    frag_col = vec4(res, 1.0);
}