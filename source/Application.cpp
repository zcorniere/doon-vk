#include "Application.hpp"

Application::Application(): window("Vulkan", 800, 600)
{
    window.setKeyCallback(Application::keyboard_callback);
    this->VulkanApplication::init(window);
}

Application::~Application() {}

void Application::run()
{
    while (!window.shouldClose()) { window.pollEvent(); }
}

void Application::keyboard_callback(GLFWwindow *win, int key, int, int action, int)
{
    switch (action) {
        case GLFW_REPEAT:
        case GLFW_PRESS: {
            switch (key) {
                case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(win, true); break;
                default: break;
            }
            default: break;
        }
    }
}