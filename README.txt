Creating a c++ wrapper for opengl.
Supposed to make it use c++ syntax better and print errors to help debugging.
Uses GLFW 3.3 and glad.

TOODS:
1. Change utils functions that generate 2d coords to gen pseudo-2d in 3d coords and compare performance.
2. Increase test items.
3. Flag to automatically log test times.
4. Auto-change sens variables based on fps.

?. Make multithreaded with distinct render thread and main thread for events
?. Make compatible with opengl 3.3 & 4.3 (current only for 4.5+)
?. Make compatible with c++17 (lowest c++14 not sure if possible)

MAYBES:
?. Use filesystem api to get filenames for test shaders