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
    Window window;
};