#include <iostream>
#include <cmath>
#include <portaudio.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>

// ImGui includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GL/glew.h>    // Initialize with glewInit()
#include <GLFW/glfw3.h>

#include "Keyboard.h"

#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 1024
#define TABLE_SIZE 200

// Simple instrument/track container
struct Instrument {
    std::string name;
    std::string waveform;
    std::vector<Voice> voices;
    std::vector<float> recorded;
    size_t playIndex;
    bool isRecording;
    bool isPlaying;
    float offsetSeconds; // start position for playback

    Instrument(const std::string& n)
        : name(n), waveform("sine"), playIndex(0),
          isRecording(false), isPlaying(false), offsetSeconds(0.0f) {}
};

// All instruments/tracks
std::vector<Instrument> instruments;
int currentInstrument = 0; // index of instrument controlled by keyboard

// Mutex protecting instruments and their data
std::mutex instrumentsMutex;

// Flag to control audio thread
std::atomic<bool> audioRunning(true);

// Flag to see if key is pressed
std::atomic<bool> keyPressed(false);

// Master volume
std::atomic<float> volume(1.0);

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    float *out = (float*) outputBuffer;
    unsigned long i;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    // Get the current volume value
    float vol = volume.load();

    for( i=0; i<framesPerBuffer; i++ )
    {
        float value = 0.0f;
        {
            std::lock_guard<std::mutex> lock(instrumentsMutex);
            for (auto &inst : instruments) {
                // Sum live voices for this instrument
                float instValue = 0.0f;
                for (auto &v : inst.voices) {
                    instValue += static_cast<float>(v.osc.getWaveformValue());
                }
                value += instValue;

                // Record this instrument if armed
                if (inst.isRecording) {
                    inst.recorded.push_back(instValue);
                }

                // Playback of recorded track
                if (inst.isPlaying) {
                    size_t offsetSamples = static_cast<size_t>(inst.offsetSeconds * SAMPLE_RATE);
                    size_t idx = inst.playIndex++;
                    if (idx >= offsetSamples && (idx - offsetSamples) < inst.recorded.size()) {
                        value += inst.recorded[idx - offsetSamples];
                    }
                    if (idx >= offsetSamples + inst.recorded.size()) {
                        inst.isPlaying = false;
                        inst.playIndex = 0;
                    }
                }
            }
        }

        value *= vol;
        *out++ = value;  /* left */
        *out++ = value;  /* right */
    }

    return paContinue;
}


void audioThread()
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;

    err = Pa_Initialize();
    if( err != paNoError ) return;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              NULL );
    if( err != paNoError ) return;

    err = Pa_StartStream( stream );
    if( err != paNoError ) return;

    while (audioRunning)
    {
        Pa_Sleep(10);
    }

    err = Pa_StopStream( stream );
    if( err != paNoError ) return;

    err = Pa_CloseStream( stream );
    if( err != paNoError ) return;

    Pa_Terminate();
}


int main()
{

    // Setup window
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui OpenGL3 example", NULL, NULL);

    glfwMakeContextCurrent(window);
    glewInit();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    // Create a default instrument
    {
        std::lock_guard<std::mutex> lock(instrumentsMutex);
        instruments.emplace_back("Instrument 1");
    }

    // Start audio processing in a separate thread
    std::thread audio(audioThread);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Route keyboard to current instrument
        if (!instruments.empty()) {
            Keyboard(window, instruments[currentInstrument].voices, instrumentsMutex, keyPressed,
                     instruments[currentInstrument].waveform);
        }
        glfwSetKeyCallback(window, key_callback);

        ImGui::Begin("Music Editor");
        if (ImGui::Button("Add Instrument")) {
            std::lock_guard<std::mutex> lock(instrumentsMutex);
            std::string name = "Instrument " + std::to_string(instruments.size() + 1);
            instruments.emplace_back(name);
        }

        std::lock_guard<std::mutex> lock(instrumentsMutex);
        if (!instruments.empty()) {
            std::vector<const char*> names;
            names.reserve(instruments.size());
            for (auto& inst : instruments) names.push_back(inst.name.c_str());
            ImGui::Combo("Current Instrument", &currentInstrument, names.data(), names.size());

            Instrument &inst = instruments[currentInstrument];
            const char* items[] = { "sine", "square", "sawtooth", "triangle", "noise" };
            int currentItem = 0;
            for (int i = 0; i < 5; ++i) {
                if (inst.waveform == items[i]) currentItem = i;
            }
            if (ImGui::Combo("Waveform", &currentItem, items, IM_ARRAYSIZE(items))) {
                inst.waveform = items[currentItem];
                for (auto &v : inst.voices) {
                    v.osc.setWaveform(inst.waveform);
                }
            }

            if (!inst.isRecording) {
                if (ImGui::Button("Record")) {
                    inst.recorded.clear();
                    inst.playIndex = 0;
                    inst.isPlaying = false;
                    inst.isRecording = true;
                }
            } else {
                ImGui::Text("Recording...");
                if (ImGui::Button("Stop")) {
                    inst.isRecording = false;
                }
            }

            ImGui::SliderFloat("Start Offset (s)", &inst.offsetSeconds, 0.0f, 10.0f);

            if (ImGui::Button("Clear Track")) {
                inst.recorded.clear();
                inst.playIndex = 0;
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Master Play")) {
            for (auto &inst : instruments) {
                if (!inst.recorded.empty()) {
                    inst.playIndex = 0;
                    inst.isPlaying = true;
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Master Stop")) {
            for (auto &inst : instruments) {
                inst.isPlaying = false;
                inst.playIndex = 0;
            }
        }
        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    // Stop the audio thread
    audioRunning = false;
    audio.join();

    return 0;
}
