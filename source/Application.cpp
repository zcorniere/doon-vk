#include "Application.hpp"
#include "Logger.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>

#include <chrono>

Application::Application()
{
    window.setUserPointer(this);
    window.captureCursor(true);
    window.setKeyCallback(Application::keyboard_callback);
    window.setCursorPosCallback(Application::cursor_callback);
    window.setTitle(uiRessources.sWindowTitle);
    this->VulkanApplication::init();
}

Application::~Application() {}

void Application::run()
{
    float fElapsedTime = 0;

    for (unsigned i = 0; i < swapChainImages.size(); i++) { updateUniformBuffer(i); }
    while (!window.shouldClose()) {
        window.setTitle(uiRessources.sWindowTitle);
        auto tp1 = std::chrono::high_resolution_clock::now();

        window.pollEvent();
        if (!bInteractWithUi) {
            if (window.isKeyPressed(GLFW_KEY_W)) camera.processKeyboard(Camera::FORWARD, fElapsedTime);
            if (window.isKeyPressed(GLFW_KEY_S)) camera.processKeyboard(Camera::BACKWARD, fElapsedTime);
            if (window.isKeyPressed(GLFW_KEY_D)) camera.processKeyboard(Camera::RIGHT, fElapsedTime);
            if (window.isKeyPressed(GLFW_KEY_A)) camera.processKeyboard(Camera::LEFT, fElapsedTime);
            if (window.isKeyPressed(GLFW_KEY_SPACE)) camera.processKeyboard(Camera::UP, fElapsedTime);
            if (window.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) camera.processKeyboard(Camera::DOWN, fElapsedTime);
        }
        drawImgui();
        drawFrame();
        auto tp2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsedTime(tp2 - tp1);
        fElapsedTime = elapsedTime.count();
    }
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
    VK_TRY(vkResetFences(device, 1, &frame.inFlightFences));
    VK_TRY(vkResetCommandBuffer(commandBuffers[imageIndex], 0));
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

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };

    std::array<VkClearValue, 2> clearValues{};
    clearValues.at(0).color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues.at(1).depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = swapChainFramebuffers[imageIndex],
        .renderArea =
            {
                .offset = {0, 0},
                .extent = swapChainExtent,
            },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
    auto gpuCamera = camera.getGPUCameraData();
    auto &mesh = loadedMeshes.at("viking_room");
    VK_TRY(vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo));
    vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(gpuCamera), &gpuCamera);
    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkBuffer vertexBuffers[] = {mesh.meshBuffer.buffer};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[imageIndex], mesh.meshBuffer.buffer, mesh.indicesOffset,
                             VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &descriptorSets[imageIndex], 0, nullptr);

        vkCmdDrawIndexed(commandBuffers[imageIndex], mesh.indicesSize, 1, 0, 0, 0);
    }
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[imageIndex]);
    vkCmdEndRenderPass(commandBuffers[imageIndex]);
    VK_TRY(vkEndCommandBuffer(commandBuffers[imageIndex]));
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

void Application::drawImgui()
{
    const std::vector<VkSampleCountFlagBits> counts = {
        VK_SAMPLE_COUNT_1_BIT,  VK_SAMPLE_COUNT_2_BIT,  VK_SAMPLE_COUNT_4_BIT,  VK_SAMPLE_COUNT_8_BIT,
        VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_64_BIT,
    };

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Hello world");
    if (ImGui::CollapsingHeader("Menu")) {
        if (!uiRessources.bShowFpsInTitle) {
            ImGui::InputText("Window Title", uiRessources.sWindowTitle, IM_ARRAYSIZE(uiRessources.sWindowTitle));
        } else {
            snprintf(uiRessources.sWindowTitle, 128, "%f", ImGui::GetIO().Framerate);
        }
        ImGui::Checkbox("Show fps in the tile", &uiRessources.bShowFpsInTitle);
    }
    if (ImGui::Checkbox("Wireframe mode", &uiRessources.bWireFrameMode)) {
        creationParameters.polygonMode =
            (uiRessources.bWireFrameMode) ? (VK_POLYGON_MODE_LINE) : (VK_POLYGON_MODE_FILL);
        framebufferResized = true;
    }
    if (ImGui::BeginCombo("##sample_count", vk_utils::tools::to_string(creationParameters.msaaSample).c_str())) {
        for (const auto &msaa: counts) {
            bool is_selected = (creationParameters.msaaSample == msaa);
            if (ImGui::Selectable(vk_utils::tools::to_string(msaa).c_str(), is_selected)) {
                creationParameters.msaaSample = msaa;
                framebufferResized = true;
            }
            if (is_selected) { ImGui::SetItemDefaultFocus(); }
            if (msaa == maxMsaaSample) break;
        }
        ImGui::EndCombo();
    }

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::End();
}

void Application::updateUniformBuffer(uint32_t currentImage)
{
    /* static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count(); */

    UniformBufferObject ubo{
        .translation = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, -0.6f, 0.0f)),
        .rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        .scale = glm::scale(glm::mat4{1.0f}, glm::vec3(2.0f)),
    };
    void *mapped = nullptr;
    vmaMapMemory(allocator, uniformBuffers[currentImage].memory, &mapped);
    std::memcpy(mapped, &ubo, sizeof(ubo));
    vmaUnmapMemory(allocator, uniformBuffers[currentImage].memory);
}

void Application::keyboard_callback(GLFWwindow *win, int key, int, int action, int)
{
    auto *eng = (Application *)glfwGetWindowUserPointer(win);

    switch (action) {
        case GLFW_REPEAT:
        case GLFW_PRESS: {
            switch (key) {
                case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(win, true); break;
                case GLFW_KEY_LEFT_ALT:
                    if (eng->bInteractWithUi) {
                        eng->window.captureCursor(true);
                        eng->bInteractWithUi = false;
                    } else {
                        eng->window.captureCursor(false);
                        eng->bInteractWithUi = true;
                    }
                    break;
                default: break;
            }
            default: break;
        }
    }
}

void Application::cursor_callback(GLFWwindow *win, double xpos, double ypos)
{
    auto *eng = (Application *)glfwGetWindowUserPointer(win);
    if (eng->bInteractWithUi) return;

    auto xoffset = xpos - eng->lastX;
    auto yoffset = eng->lastY - ypos;

    eng->lastX = xpos;
    eng->lastY = ypos;
    eng->camera.processMouseMovement(xoffset, yoffset);
}
