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
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    VK_TRY(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

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
    vkQueuePresentKHR(presentQueue, &presentInfo);
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