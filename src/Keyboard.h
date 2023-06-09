#ifndef KEYBOARD_H
#define KEYBOARD_H

#pragma once

#include "Oscillator.h"
#include <GLFW/glfw3.h>
#include <atomic>

// Function for handling keyboard input for notes
void Keyboard(GLFWwindow* window, Oscillator& oscillator, std::atomic<bool>& keyPressed);

// Callback function for handling octave change keys only
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Octave variable shared between the callback and Keyboard functions
extern int octave;

#endif // KEYBOARD_H
