#pragma once

#include "VulkanApplication.hpp"
#include "memory"

class Application : public VulkanApplication
{
public:
    Application();
    ~Application();

    void run();

private:
    void drawFrame();
    static void keyboard_callback(GLFWwindow *win, int key, int, int action, int);

private:
};