#include "GUI.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

GUI::GUI(GLFWwindow* window, Oscillator& osc, std::atomic<bool>& keyPressed)
    : window(window), osc(osc), keyPressed(keyPressed), currentItem(0)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    setupImGuiStyle();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

GUI::~GUI()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GUI::setupImGuiStyle()
{
    ImGui::StyleColorsDark();
}

void GUI::Render()
{
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("SynthWave Oscillator");

        ImGui::Text("Select the waveform");
        const char* items[] = { "sine", "square", "sawtooth", "triangle" };
        if(ImGui::Combo("waveform", &currentItem, items, IM_ARRAYSIZE(items))) {
            std::lock_guard<std::mutex> lock(waveformMutex);
            waveformName = items[currentItem];
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

void GUI::updateWaveform()
{
    if (!keyPressed)
    {
        osc.setWaveform("none");
    }
    else
    {
        osc.setWaveform(waveformName);
    }
}

