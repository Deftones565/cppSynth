#include "imgui.h"
#include "Keyboard.h"
#include "Oscillator.h"
#include <GLFW/glfw3.h>

int octave = 0;

void Keyboard(GLFWwindow* window, Oscillator& oscillator, std::atomic<bool>& keyPressed) {
    ImGuiIO& io = ImGui::GetIO();
    
    // Process key press event
    glfwPollEvents();
    for (int i = 0; i < GLFW_KEY_LAST; i++) {
        if (glfwGetKey(window, i) == GLFW_PRESS) {
            keyPressed = true;
            switch (i) {
                // note keys
                case GLFW_KEY_A: oscillator.setNote(60 + octave * 12); break; // C4
                case GLFW_KEY_S: oscillator.setNote(62 + octave * 12); break; // D4
                case GLFW_KEY_D: oscillator.setNote(64 + octave * 12); break; // E4
                case GLFW_KEY_F: oscillator.setNote(65 + octave * 12); break; // F4
                case GLFW_KEY_G: oscillator.setNote(67 + octave * 12); break; // G4
                case GLFW_KEY_H: oscillator.setNote(69 + octave * 12); break; // A4
                case GLFW_KEY_J: oscillator.setNote(71 + octave * 12); break; // B4
                case GLFW_KEY_K: oscillator.setNote(72 + octave * 12); break; // C5
                case GLFW_KEY_L: oscillator.setNote(74 + octave * 12); break; // D5
                case GLFW_KEY_SEMICOLON: oscillator.setNote(76 + octave * 12); break; // E5

	    }
        }
    }
    
    // Process key release event
    if (!glfwGetKey(window, GLFW_KEY_A) &&
        !glfwGetKey(window, GLFW_KEY_S) &&
        !glfwGetKey(window, GLFW_KEY_D) &&
        !glfwGetKey(window, GLFW_KEY_F) &&
        !glfwGetKey(window, GLFW_KEY_G) &&
        !glfwGetKey(window, GLFW_KEY_H) &&
        !glfwGetKey(window, GLFW_KEY_J) &&
        !glfwGetKey(window, GLFW_KEY_K) &&
        !glfwGetKey(window, GLFW_KEY_L) &&
        !glfwGetKey(window, GLFW_KEY_SEMICOLON)) {
        keyPressed = false;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_UP: if (octave < 2) ++octave; break; // Increase octave if less than 2
            case GLFW_KEY_DOWN: if (octave > -2) --octave; break; // Decrease octave if more than -2
        }
    }
}


