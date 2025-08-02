#include "imgui.h"
#include "Keyboard.h"
#include <GLFW/glfw3.h>
#include <algorithm>

// Constants for constructing voices
static const unsigned kTableSize = 200;
static const double kSampleRate = 48000.0;

int octave = 0;

// Map of keys to MIDI notes for one octave starting at C4
struct KeyMap { int key; int note; };
static const KeyMap keyMap[] = {
    {GLFW_KEY_A, 60}, // C4
    {GLFW_KEY_S, 62}, // D4
    {GLFW_KEY_D, 64}, // E4
    {GLFW_KEY_F, 65}, // F4
    {GLFW_KEY_G, 67}, // G4
    {GLFW_KEY_H, 69}, // A4
    {GLFW_KEY_J, 71}, // B4
    {GLFW_KEY_K, 72}, // C5
    {GLFW_KEY_L, 74}, // D5
    {GLFW_KEY_SEMICOLON, 76} // E5
};

void Keyboard(GLFWwindow* window, std::vector<Voice>& voices, std::mutex& voiceMutex,
              std::atomic<bool>& keyPressed, const std::string& waveform) {
    static bool keysDownPrev[GLFW_KEY_LAST] = {false};

    glfwPollEvents();

    for (const auto& km : keyMap) {
        bool isDown = glfwGetKey(window, km.key) == GLFW_PRESS;
        bool wasDown = keysDownPrev[km.key];
        int note = km.note + octave * 12;

        if (isDown && !wasDown) {
            Voice v(kTableSize, kSampleRate);
            v.note = note;
            v.osc.setWaveform(waveform);
            v.osc.setNote(note);
            std::lock_guard<std::mutex> lock(voiceMutex);
            voices.push_back(std::move(v));
            keysDownPrev[km.key] = true;
        }
        if (!isDown && wasDown) {
            std::lock_guard<std::mutex> lock(voiceMutex);
            voices.erase(std::remove_if(voices.begin(), voices.end(),
                                        [note](const Voice& v){ return v.note == note; }),
                          voices.end());
            keysDownPrev[km.key] = false;
        }
    }

    {
        std::lock_guard<std::mutex> lock(voiceMutex);
        keyPressed = !voices.empty();
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
