Just going through learning opengl. https://learnopengl.com/
Creating a c++ wrapper for opengl.
Supposed to make it use c++ syntax better and print errors to help debugging.
Uses GLFW 3.3 and glad.
Utils uses stb_image, stb_true_type as wrappers for STB_IMAGE and STB_TRUETYPE from https://github.com/nothings/stb by Sean Barrette.

TOODS:
1. Change utils functions that generate 2d coords to gen pseudo-2d in 3d coords and compare performance.
2. Auto-change sens variables based on fps.
3. Change references to const char * to also allow std::string_view and convertibles to it.
4. Utils hex should allow #aaa and such and #rrggbbaa and such

?. Make compatible with opengl 3.3 & 4.3 (current only for 4.5+)
?. Make compatible with c++17 (lowest c++14 not sure if possible)

MAYBES:
?. Use filesystem api to get filenames for test shaders