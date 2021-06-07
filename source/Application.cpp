#include "Application.hpp"

Application::Application(): window("Vulkan", 800, 600)
{
    window.setKeyCallback(Application::keyboard_callback);
    this->VulkanApplication::init(window);
}

Application::~Application() {}

void Application::run()
{
    while (!window.shouldClose()) {
        window.pollEvent();
        drawFrame();
        vkDeviceWaitIdle(device);
    }
}

void Application::drawFrame()
{
    auto &frame = frames[currentFrame];
    uint32_t imageIndex;

    VK_TRY(vkWaitForFences(device, 1, &frame.inFlightFences, VK_TRUE, UINT64_MAX));
    VK_TRY(vkResetFences(device, 1, &frame.inFlightFences));
    VK_TRY(vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, frame.imageAvailableSemaphore, nullptr, &imageIndex));

    VkSemaphore waitSemaphores[] = {frame.imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {frame.renderFinishedSemaphore};
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };
    VK_TRY(vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.inFlightFences));

    VkSwapchainKHR swapChains[] = {swapChain};
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };
    VK_TRY(vkQueuePresentKHR(presentQueue, &presentInfo));
    currentFrame = (currentFrame + 1) % MAX_FRAME_FRAME_IN_FLIGHT;
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