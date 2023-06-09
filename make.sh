#!/bin/bash

# Script to compile the C++ Synth project

# Make bin folder
mkdir bin

# Compile the C++ code
g++ -std=c++11 \
    src/main.cpp \
    src/Oscillator.cpp \
    src/Keyboard.cpp \
    ./lib/imgui/*.cpp \
    ./lib/imgui/backends/imgui_impl_glfw.cpp \
    ./lib/imgui/backends/imgui_impl_opengl3.cpp \
    -o ./bin/synth \
    -I./lib/imgui \
    -I./lib/imgui/backends \
    -I./lib \
    -L./lib \
    -lportaudio \
    -lglfw \
    -lGLEW \
    -lGL \
    -ldl \
    -lpthread

# Explanation of the options used:
# -std=c++11: Specifies the C++ language version to use.
# src/main.cpp, src/Oscillator.cpp, src/Keyboard.cpp: Source files to compile.
# ./lib/imgui/*.cpp: ImGui library source files.
# ./lib/imgui/backends/imgui_impl_glfw.cpp: ImGui GLFW backend source file.
# ./lib/imgui/backends/imgui_impl_opengl3.cpp: ImGui OpenGL3 backend source file.
# -o ./bin/synth: Output binary file name and location.
# -I./lib/imgui, -I./lib/imgui/backends, -I./lib: Include directories for header files.
# -L./lib: Library directory.
# -lportaudio, -lglfw, -lGLEW, -lGL, -ldl, -lpthread: Linked libraries.

# End of script

