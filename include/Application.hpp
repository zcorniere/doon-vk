#pragma once

#include <vector>

#include "DeletionQueue.hpp"
#include "Player.hpp"
#include "VulkanApplication.hpp"
#include "types/Material.hpp"
#include "types/Scene.hpp"

#define WINDOW_TITLE_MAX_SIZE 128

class Application : public VulkanApplication
{
public:
    Application();
    ~Application();

    void run();
    void loadModel();
    void loadTextures();

public:
    double lastX = 400;
    double lastY = 300;
    double yaw = -90;
    double pitch = 0;
    bool bInteractWithUi = false;

private:
    void buildIndirectBuffers(Frame &frame);
    void drawFrame();
    void drawImgui();
    static void keyboard_callback(GLFWwindow *win, int key, int, int action, int);
    static void cursor_callback(GLFWwindow *win, double xpos, double ypos);

private:
    DeletionQueue applicationDeletionQueue;
    struct {
        struct {
            float fFOV = 70.f;
            float fCloseClippingPlane = 0.1;
            float fFarClippingPlane = MAX_PROJECTION_LIMIT;
            bool bFlyingCam = false;
            float fGravity = 3.0f;
        } cameraParamettersOverride;
        char sWindowTitle[WINDOW_TITLE_MAX_SIZE] = "Vulkan";
        bool bShowFpsInTitle = false;
        bool bWireFrameMode = false;
        bool bTmpObject = false;
        std::array<float, 4> vClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    } uiRessources = {};

    Player player;
    Scene scene;
    std::vector<gpuObject::Material> materials;
    bool firstMouse = true;
};
