#include <iostream>

#define WRAP_G_OPENGL_VERSION_MAJOR 4
#define WRAP_G_OPENGL_VERSION_MINOR 6
#define WRAP_G_DEBUG true

// #include "src/wrap_g.hpp"
#include "tests/triangle.hpp"

int main()
{
    wrap_tests::create_triangle();
    
    return 0;
}