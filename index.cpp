#include <iostream>

#define WRAP_G_OPENGL_VERSION_MAJOR 4
#define WRAP_G_OPENGL_VERSION_MINOR 6
#define WRAP_G_BACKGROUND_RESOURCE_LOAD false
#define WRAP_G_MULTITHREADING false
#define WRAP_G_DEBUG true

// #include "src/wrap_g.hpp"
// #include "tests/triangle.hpp"
#include "tests/textured_rect.hpp"

int main()
{
    // wrap_tests::create_triangle();
    wrap_tests::create_textured_rect();

    return 0;
}