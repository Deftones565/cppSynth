#pragma once

#include <GLFW/glfw3.h>
#include "Oscillator.h"

class GUI {
public:
    GUI(GLFWwindow* window, Oscillator& osc, std::atomic<bool>& keyPressed);
    ~GUI();
    void Render();

private:
    GLFWwindow* window;
    Oscillator& osc;
    std::atomic<bool>& keyPressed;
    std::string waveformName;
    std::mutex waveformMutex;
    int currentItem;

    void updateWaveform();
    void setupImGuiStyle();
};

