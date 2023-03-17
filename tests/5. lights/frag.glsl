#version 450 core

in vec3 frag_pos;
in vec3 normals;
in vec2 tex_coord;

out vec4 frag_col;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light {
    vec4 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform Light light;
uniform vec3 cam_pos;

void main()
{
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, tex_coord));

    vec3 norm = normalize(normals);
    vec3 light_dir = step(1.0, light.position.w) * normalize(light.position.xyz - frag_pos)
                    + (1.0 - step(1.0, light.position.w)) * normalize(- light.position.xyz);

    vec3 diffuse = max(dot(norm, light_dir), 0.0) * light.diffuse * vec3(texture(material.diffuse, tex_coord));

    vec3 view_dir = normalize(cam_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular = spec * light.specular * vec3(texture(material.specular, tex_coord));

    vec3 res = (ambient + diffuse + specular);
    frag_col = vec4(res, 1.0);
}
