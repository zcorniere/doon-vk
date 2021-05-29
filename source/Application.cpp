#include "Application.hpp"

Application::Application(): window("Vulkan", 800, 600) { this->VulkanApplication::init(window); }

Application::~Application() {}

void Application::run()
{
    while (!window.shouldClose()) { window.pollEvent(); }
}