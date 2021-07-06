#pragma once

#include "VulkanApplication.hpp"
#include "types/RenderObject.hpp"
#include <vector>

#define WINDOW_TITLE_MAX_SIZE 128

class Application : public VulkanApplication
{
public:
    Application();
    ~Application();

    void run();

public:
    double lastX = 400;
    double lastY = 300;
    double yaw = -90;
    double pitch = 0;
    bool bInteractWithUi = false;

private:
    void drawFrame();
    void drawImgui();
    static void keyboard_callback(GLFWwindow *win, int key, int, int action, int);
    static void cursor_callback(GLFWwindow *win, double xpos, double ypos);

private:
    struct {
        char sWindowTitle[WINDOW_TITLE_MAX_SIZE] = "Vulkan";
        bool bShowFpsInTitle = false;
        bool bWireFrameMode = false;
    } uiRessources = {};
    Camera camera;

    std::vector<RenderObject> sceneModels;
    std::vector<gpuObject::Material> materials;

    bool firstMouse = true;
};
