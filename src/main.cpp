#include <iostream>
#include <cmath>
#include <portaudio.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <algorithm>

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
    float volume;        // per-track volume
    bool mute;           // track mute state

    Instrument(const std::string& n)
        : name(n), waveform("sine"), playIndex(0),
          isRecording(false), isPlaying(false), offsetSeconds(0.0f),
          volume(1.0f), mute(false) {}
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

                // Record raw waveform before applying volume/mute
                if (inst.isRecording) {
                    inst.recorded.push_back(instValue);
                }

                float instGain = inst.mute ? 0.0f : inst.volume;
                value += instValue * instGain;

                // Playback of recorded track
                if (inst.isPlaying) {
                    size_t offsetSamples = static_cast<size_t>(inst.offsetSeconds * SAMPLE_RATE);
                    size_t idx = inst.playIndex++;
                    if (idx >= offsetSamples && (idx - offsetSamples) < inst.recorded.size()) {
                        value += inst.recorded[idx - offsetSamples] * instGain;
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

        float masterVol = volume.load();
        if (ImGui::SliderFloat("Master Volume", &masterVol, 0.0f, 1.0f)) {
            volume.store(masterVol);
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

            ImGui::SliderFloat("Track Volume", &inst.volume, 0.0f, 1.0f);
            ImGui::Checkbox("Mute", &inst.mute);

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

        ImGui::Separator();
        ImGui::Text("Timeline");
        if (!instruments.empty()) {
            const float trackHeight = 60.0f;
            float timelineWidth = ImGui::GetContentRegionAvail().x - 10.0f;
            float maxLength = 10.0f;
            for (auto &inst : instruments) {
                float len = inst.offsetSeconds + inst.recorded.size() / static_cast<float>(SAMPLE_RATE);
                maxLength = std::max(maxLength, len);
            }
            float snap = 0.25f; // seconds
            ImVec2 startPos = ImGui::GetCursorScreenPos();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            float totalHeight = trackHeight * instruments.size();

            int divs = static_cast<int>(maxLength / snap);
            for (int i = 0; i <= divs; ++i) {
                float x = startPos.x + (i * timelineWidth) / divs;
                drawList->AddLine(ImVec2(x, startPos.y), ImVec2(x, startPos.y + totalHeight), IM_COL32(80,80,80,255));
                if (i % 4 == 0) {
                    std::string t = std::to_string(i * snap);
                    drawList->AddText(ImVec2(x + 2, startPos.y - 15), IM_COL32(255,255,255,255), t.c_str());
                }
            }

            static int draggedTrack = -1;
            for (size_t idx = 0; idx < instruments.size(); ++idx) {
                Instrument &inst = instruments[idx];
                float y = startPos.y + idx * trackHeight;
                drawList->AddRectFilled(ImVec2(startPos.x, y), ImVec2(startPos.x + timelineWidth, y + trackHeight), IM_COL32(50,50,50,200));
                float trackStart = startPos.x + (inst.offsetSeconds / maxLength) * timelineWidth;
                float trackLenSec = inst.recorded.size() / static_cast<float>(SAMPLE_RATE);
                float trackWidth = (trackLenSec / maxLength) * timelineWidth;
                ImVec2 rectMin(trackStart, y + 5);
                ImVec2 rectMax(trackStart + trackWidth, y + trackHeight - 5);
                drawList->AddRectFilled(rectMin, rectMax, IM_COL32(100,150,240,255));
                drawList->AddRect(rectMin, rectMax, IM_COL32(255,255,255,255));

                if (!inst.recorded.empty() && trackWidth > 0) {
                    float midY = (rectMin.y + rectMax.y) * 0.5f;
                    for (int x = 0; x < (int)trackWidth; ++x) {
                        size_t idxSample = static_cast<size_t>((float)x / trackWidth * inst.recorded.size());
                        float s = inst.recorded[idxSample];
                        drawList->AddLine(ImVec2(rectMin.x + x, midY),
                                          ImVec2(rectMin.x + x, midY - s * (trackHeight/2 - 5)),
                                          IM_COL32(255,255,255,100));
                    }
                }

                ImGui::SetCursorScreenPos(rectMin);
                ImGui::InvisibleButton(("track" + std::to_string(idx)).c_str(), ImVec2(trackWidth, trackHeight - 10));
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    float delta = ImGui::GetIO().MouseDelta.x;
                    float deltaSeconds = delta / timelineWidth * maxLength;
                    inst.offsetSeconds = std::max(0.0f, inst.offsetSeconds + deltaSeconds);
                    draggedTrack = (int)idx;
                }
                if (draggedTrack == (int)idx && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    inst.offsetSeconds = std::round(inst.offsetSeconds / snap) * snap;
                    draggedTrack = -1;
                }
            }
            ImGui::Dummy(ImVec2(timelineWidth, totalHeight));
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
