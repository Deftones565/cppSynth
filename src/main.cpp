#include <iostream>
#include <cmath>
#include <portaudio.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

// ImGui includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GL/glew.h>    // Initialize with glewInit()
#include <GLFW/glfw3.h>

#include "Oscillator.h"
#include "Keyboard.h"

#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 1024
#define TABLE_SIZE 200

// Keeps track of the previous key states on the keyboard so I can only have sound playing when key is pressed and unpressed
bool keysDownPrev[512] = { false };

// Waveform selection shared between threads
std::string waveformName = "sine";

// Mutex for protecting access to waveformName
std::mutex waveformMutex;

// Oscillator instance
Oscillator osc(TABLE_SIZE, SAMPLE_RATE);

// Flag to control audio thread
std::atomic<bool> audioRunning(true);

// Flag to see if key is pressed
std::atomic<bool> keyPressed(false);

// Volume stuff I guess
std::atomic<float> volume(1.0);

// Recording and playback state
std::vector<float> recordedSamples;
std::mutex recordMutex;
std::atomic<bool> isRecording(false);
std::atomic<bool> isPlaying(false);
std::atomic<bool> isLooping(false);
size_t playIndex = 0;

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

    std::lock_guard<std::mutex> lock(recordMutex);
    bool recording = isRecording.load();
    bool playing = isPlaying.load();

    for( i=0; i<framesPerBuffer; i++ )
    {
        float value = 0.0f;
        if (playing && playIndex < recordedSamples.size()) {
            value = recordedSamples[playIndex++];
            if (playIndex >= recordedSamples.size()) {
                if (isLooping.load()) {
                    playIndex = 0;
                } else {
                    isPlaying = false;
                    playIndex = 0;
                }
            }
        } else {
            value = osc.getWaveformValue();
            if (recording) {
                recordedSamples.push_back(value);
            }
        }

        value *= vol;
        *out++ = value;  /* left */
        *out++ = value;  /* right */
    }

    return paContinue;
}


void audioThread(std::atomic<bool>& keyPressed, std::string& waveform)
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
        if (!keyPressed)
        {
            osc.setWaveform("none");
        }
        else
        {
            osc.setWaveform(waveform);
        }
        Pa_Sleep(1);
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

    // Start audio processing in a separate thread
        std::thread audio(audioThread, std::ref(keyPressed), std::ref(waveformName));

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        Keyboard(window, osc, keyPressed);
	glfwSetKeyCallback(window, key_callback);

        {
            ImGui::Begin("SynthWave Oscillator");

            ImGui::Text("Select the waveform");
            const char* items[] = { "sine", "square", "sawtooth", "triangle", "noise" };
            static int currentItem = 0;
            if(ImGui::Combo("waveform", &currentItem, items, IM_ARRAYSIZE(items))) {
                std::lock_guard<std::mutex> lock(waveformMutex);
                waveformName = items[currentItem];
            }

            ImGui::Separator();
            if (!isRecording.load()) {
                if (ImGui::Button("Start Recording")) {
                    std::lock_guard<std::mutex> lock(recordMutex);
                    recordedSamples.clear();
                    playIndex = 0;
                    isPlaying = false;
                    isRecording = true;
                }
            } else {
                if (ImGui::Button("Stop Recording")) {
                    isRecording = false;
                }
            }

            if (!isPlaying.load()) {
                if (ImGui::Button("Play")) {
                    std::lock_guard<std::mutex> lock(recordMutex);
                    playIndex = 0;
                    if (!recordedSamples.empty()) {
                        isPlaying = true;
                    }
                }
            } else {
                if (ImGui::Button("Stop")) {
                    isPlaying = false;
                    playIndex = 0;
                }
            }

            bool loop = isLooping.load();
            if (ImGui::Checkbox("Loop", &loop)) {
                isLooping = loop;
            }

            ImGui::End();
        }

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
