#include <iostream>

#define WRAP_G_OPENGL_VERSION_MAJOR 4
#define WRAP_G_OPENGL_VERSION_MINOR 6
#define WRAP_G_BACKGROUND_RESOURCE_LOAD true
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

    // auto read_csv = utils::read_csv_sync<
    //     std::string,
    //     double, double, double,
    //     double, double, double,
    //     double, double, double,
    //     double
    // >("./tests/4. materials/materials list.csv", true, utils::strto);

    // if (read_csv.second.size() > 0) {
    //     std::cout << "\ndone\n";
    //     utils::print_vecs<11>(read_csv.first.data());
    //     utils::print_tuple(read_csv.second[0]);
    // }

    return 0;
}