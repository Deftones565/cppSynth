#ifndef KEYBOARD_H
#define KEYBOARD_H

#pragma once

#include "Oscillator.h"
#include <GLFW/glfw3.h>
#include <atomic>
#include <vector>
#include <mutex>
#include <string>

// Simple container for a single playing voice
struct Voice {
    Oscillator osc;
    int note;
    Voice(unsigned tableSize, double sampleRate) : osc(tableSize, sampleRate), note(0) {}
};

// Function for handling keyboard input for notes
void Keyboard(GLFWwindow* window, std::vector<Voice>& voices, std::mutex& voiceMutex,
              std::atomic<bool>& keyPressed, const std::string& waveform);

// Callback function for handling octave change keys only
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Octave variable shared between the callback and Keyboard functions
extern int octave;

#endif // KEYBOARD_H
