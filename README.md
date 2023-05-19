# What this is about

Just going through learning opengl. https://learnopengl.com/
Creating a c++ wrapper for opengl.
Supposed to make it use c++ syntax better and print errors to help debugging.
Uses GLFW 3.3 and glad.
Utils uses stb_image, stb_true_type as wrappers for STB_IMAGE and STB_TRUETYPE from https://github.com/nothings/stb by Sean Barrette.

TOODS:
1. Change utils functions that generate 2d coords to gen pseudo-2d in 3d coords and compare performance.
2. Auto-change sens variables based on fps.
3. Change references to const char * to also allow std::string_view and convertibles to it.

MAYBES:
?. Use filesystem api to get filenames for test shaders

# Getting Started

## Step 1: Clone the github repo

Go to https://github.com/Rashnan/wrap_g.git and download the zip or clone the repo using git.

## Step 2: Fix Paths

If using vsocde/vscodium fix the c++ compiler path, include path and the gdb path to your specific pc.

## Step 3: Install glad and GLFW3

Note that wrap_g only supports opengl 4.5+ and uses both glad and GLFW.
Download the appropriate glad header and source files from https://glad.dav1d.de/ and add them to the include path.

(Optional) Compile the glad.c into a library (libglad.a) and add it to the compiler linker args (add -lglad).

Download GLFW3 from https://www.glfw.org/ and also add it to the include path.

## Finish
Now start your project in the index.cpp file.