#pragma once

#include "VulkanApplication.hpp"
#include "types/RenderObject.hpp"
#include <vector>

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
    void updateUniformBuffer(uint32_t currentImage);
    static void keyboard_callback(GLFWwindow *win, int key, int, int action, int);
    static void cursor_callback(GLFWwindow *win, double xpos, double ypos);

private:
    struct {
        char sWindowTitle[128] = "Vulkan";
        bool bShowFpsInTitle = false;
        bool bWireFrameMode = false;
    } uiRessources = {};
    Camera camera;

    std::vector<RenderObject> sceneModels;
    bool firstMouse = true;
};
