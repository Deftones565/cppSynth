#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Gui {
public:
    Gui();
    ~Gui();

    void InitializeAllGui();
    void SetupWindow();
    void SetWindow(GLFWwindow* setWindow);
    void GetWindow() const;
    void SetupImGui();
    void Cleanup();

private:
    GLFWwindow* window;
};
