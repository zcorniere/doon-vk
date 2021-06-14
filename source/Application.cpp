#include "Application.hpp"
#include "Logger.hpp"
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

Application::Application()
{
    window.setKeyCallback(Application::keyboard_callback);
    this->VulkanApplication::init();
}

Application::~Application() {}

void Application::run()
{
    while (!window.shouldClose()) {
        window.pollEvent();
        drawFrame();
    }
    vkDeviceWaitIdle(device);
}

void Application::drawFrame()
{
    auto &frame = frames[currentFrame];
    uint32_t imageIndex;

    VK_TRY(vkWaitForFences(device, 1, &frame.inFlightFences, VK_TRUE, UINT64_MAX));

    VkResult result =
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, frame.imageAvailableSemaphore, nullptr, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw VulkanException(result);
    }

    VkSemaphore waitSemaphores[] = {frame.imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {frame.renderFinishedSemaphore};

    updateUniformBuffer(imageIndex);
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

    VK_TRY(vkResetFences(device, 1, &frame.inFlightFences));
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
    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        return recreateSwapchain();
    } else {
        VK_TRY(result);
    }
    currentFrame = (currentFrame + 1) % MAX_FRAME_FRAME_IN_FLIGHT;
}

void Application::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{
        .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj =
            glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f),
    };
    ubo.proj[1][1] *= -1;

    void *data = nullptr;
    vkMapMemory(device, uniformBufferMemory[currentImage], 0, sizeof(ubo), 0, &data);
    std::memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBufferMemory[currentImage]);
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
