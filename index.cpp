#include <vector>
#include <iostream>
#include <future>

#define WRAP_G_OPENGL_VERSION_MAJOR 4
#define WRAP_G_OPENGL_VERSION_MINOR 6
#define WRAP_G_BACKGROUND_RESOURCE_LOAD true
#define WRAP_G_DEBUG true

#define WRAP_G_USE_NEW_OPENGL_DEBUG_MESSAGE_CONTROL true

// #include "src/wrap_g.hpp"

// #include "tests/1. triangle/triangle.hpp"
// #include "tests/2. textured rect/textured_rect.hpp"
// #include "tests/3. moving around cubes/moving_around_cubes.hpp"
// #include "tests/4. materials/materials.hpp"
#include "tests/5. lights/lights.hpp"

int main()
{
    // wrap_tests::create_triangle();
    // wrap_tests::create_textured_rect();
    // wrap_tests::create_moving_around_cubes();
    // wrap_tests::create_materials();
    wrap_tests::create_lights();

    return 0;
}