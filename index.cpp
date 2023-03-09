#include <iostream>

#define WRAP_G_OPENGL_VERSION_MAJOR 4
#define WRAP_G_OPENGL_VERSION_MINOR 6
#define WRAP_G_BACKGROUND_RESOURCE_LOAD false
#define WRAP_G_DEBUG true

// #define WRAP_G_USE_NEW_OPENGL_DEBUG_MESSAGE_CONTROL true

// #include "src/wrap_g.hpp"

// #include "tests/1. triangle/triangle.hpp"
// #include "tests/2. textured rect/textured_rect.hpp"
// #include "tests/3. moving around cubes/moving_around_cubes.hpp"
#include "tests/4. materials/materials.hpp"

int main()
{
    // wrap_tests::create_triangle();
    // wrap_tests::create_textured_rect();
    // wrap_tests::create_moving_around_cubes();
    wrap_tests::create_materials();

    // constexpr const char *materials_list_path = "./tests/4. materials/materials list.csv";
    
    // struct Material {
    //     std::string name;
    //     glm::vec3 ambient;
    //     glm::vec3 diffuse;
    //     glm::vec3 specular;
    //     float shininess;
    // };

    // auto pair = utils::read_csv_struct_sync<Material>(materials_list_path, true, [](const std::vector<std::string>& params){
    //     utils::print_vecs<11>(params.data());
    //     return Material{
    //         .name = params[0],
    //         .ambient = glm::vec3{std::stof(params[1]), std::stof(params[2]), std::stof(params[3])},
    //         .diffuse = glm::vec3{std::stof(params[4]), std::stof(params[5]), std::stof(params[6])},
    //         .specular = glm::vec3{std::stof(params[7]), std::stof(params[8]), std::stof(params[9])},
    //         .shininess = std::stof(params[10]),
    //     };
    // });

    // std::cout << "Headers:\n";
    // utils::print_vecs<11>(pair.first.data());

    // std::cout << "Data:\n";
    // for (size_t i = 0; i < pair.second.size(); ++i)
    // {
    //     const auto& mat = pair.second[i];
    //     std::cout << "name: " << mat.name << ", ambient: {" << mat.ambient.r << ", " << mat.ambient.g << ",  " << mat.ambient.g << "}, diffuse: {" << mat.diffuse.r << ", " << mat.diffuse.g << ",  " << mat.diffuse.g << "}, specular: {" << mat.specular.r << ", " << mat.specular.g << ",  " << mat.specular.g << "}, shininess: " << mat.shininess << "\n";
    // }
    // std::cout << "\n\nDone...";

    return 0;
}