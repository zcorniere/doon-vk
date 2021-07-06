#include "Application.hpp"
#include "Logger.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <chrono>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>

Application::Application(): camera(glm::vec3(0.0f, 2.0f, 0.0f))
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

    sceneModels.push_back({
        .meshID = "plane",
        .ubo =
            {
                .transform =
                    {
                        .translation = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, 0.0f, 0.0f)),
                        .rotation = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                        .scale = glm::scale(glm::mat4{1.0f}, glm::vec3(50.0f)),
                    },
                .textureIndex = 0,
            },
    });
    sceneModels.push_back({
        .meshID = "viking_room",
        .ubo =
            {
                .transform =
                    {
                        .translation = glm::translate(glm::mat4{1.0f}, glm::vec3(10.0f, 0.0f, 0.0f)),
                        .rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                        .scale = glm::scale(glm::mat4{1.0f}, glm::vec3(2.0f)),
                    },
                .textureIndex = 1,
            },
    });
    sceneModels.push_back({
        .meshID = "cube",
        .ubo =
            {
                .transform =
                    {
                        .translation = glm::translate(glm::mat4{1.0f}, glm::vec3(-10.0f, 1.f, 0.0f)),
                        .rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                        .scale = glm::scale(glm::mat4{1.0f}, glm::vec3(2.0f)),
                    },
                .textureIndex = 0,
            },
    });

    materials.push_back({
        .ambientColor = {1.0f, 1.0f, 1.0f},
        .diffuse = {1.0f, 1.0f, 1.0f},
        .specular = {1.0f, 1.0f, 1.0f},
        .shininess = 0.0f,
    });

    void *objectData = nullptr;
    for (auto &frame: frames) {
        vmaMapMemory(allocator, frame.data.materialBuffer.memory, &objectData);
        gpuObject::Material *objectSSBI = (gpuObject::Material *)objectData;
        for (unsigned i = 0; i < materials.size(); i++) { objectSSBI[i] = materials.at(i); }
        vmaUnmapMemory(allocator, frame.data.materialBuffer.memory);
    }
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
    VkResult result = vkAcquireNextImageKHR(device, swapchain.getSwapchain(), UINT64_MAX, frame.imageAvailableSemaphore,
                                            nullptr, &imageIndex);

    if (vk_utils::isSwapchainInvalid(result, VK_ERROR_OUT_OF_DATE_KHR)) { return recreateSwapchain(); }

    auto &cmd = commandBuffers[imageIndex];

    VK_TRY(vkResetFences(device, 1, &frame.inFlightFences));
    VK_TRY(vkResetCommandBuffer(cmd, 0));
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
        .pCommandBuffers = &cmd,
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

    void *objectData = nullptr;
    vmaMapMemory(allocator, frame.data.uniformBuffers.memory, &objectData);
    gpuObject::UniformBufferObject *objectSSBI = (gpuObject::UniformBufferObject *)objectData;
    for (unsigned i = 0; i < sceneModels.size(); i++) { objectSSBI[i] = sceneModels.at(i).ubo; }
    vmaUnmapMemory(allocator, frame.data.uniformBuffers.memory);

    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = swapChainFramebuffers[imageIndex],
        .renderArea =
            {
                .offset = {0, 0},
                .extent = swapchain.getSwapchainExtent(),
            },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
    auto gpuCamera = camera.getGPUCameraData();
    VK_TRY(vkBeginCommandBuffer(cmd, &beginInfo));
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frame.data.objectDescriptor, 0,
                            nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &texturesSet, 0, nullptr);
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(gpuCamera), &gpuCamera);
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        for (unsigned i = 0; i < sceneModels.size(); i++) {
            const auto &mesh = loadedMeshes[sceneModels.at(i).meshID];
            VkBuffer vertexBuffers[] = {mesh.meshBuffer.buffer};
            VkDeviceSize offsets[] = {0};

            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmd, mesh.meshBuffer.buffer, mesh.indicesOffset, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmd, mesh.indicesSize, 1, 0, 0, i);
        }
    }
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRenderPass(cmd);
    VK_TRY(vkEndCommandBuffer(cmd));
    VK_TRY(vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.inFlightFences));

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = &(swapchain.getSwapchain()),
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };
    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (vk_utils::isSwapchainInvalid(result, VK_ERROR_OUT_OF_DATE_KHR, VK_SUBOPTIMAL_KHR) || framebufferResized) {
        framebufferResized = false;
        return recreateSwapchain();
    }

    currentFrame = (currentFrame + 1) % MAX_FRAME_FRAME_IN_FLIGHT;
}

void Application::drawImgui()
{
    static const std::vector<VkSampleCountFlagBits> sampleCount = {
        VK_SAMPLE_COUNT_1_BIT,  VK_SAMPLE_COUNT_2_BIT,  VK_SAMPLE_COUNT_4_BIT,  VK_SAMPLE_COUNT_8_BIT,
        VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_64_BIT,
    };
    static const std::vector<VkCullModeFlags> cullMode = {
        VK_CULL_MODE_NONE,
        VK_CULL_MODE_BACK_BIT,
        VK_CULL_MODE_FRONT_BIT,
        VK_CULL_MODE_FRONT_AND_BACK,
    };

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Hello world");
    if (ImGui::CollapsingHeader("Menu")) {
        if (!uiRessources.bShowFpsInTitle) {
            ImGui::InputText("Window Title", uiRessources.sWindowTitle, IM_ARRAYSIZE(uiRessources.sWindowTitle));
        } else {
            snprintf(uiRessources.sWindowTitle, WINDOW_TITLE_MAX_SIZE, "%f", ImGui::GetIO().Framerate);
        }
        ImGui::Checkbox("Show fps in the tile", &uiRessources.bShowFpsInTitle);
    }

    if (ImGui::CollapsingHeader("Render")) {
        if (ImGui::Checkbox("Wireframe mode", &uiRessources.bWireFrameMode)) {
            creationParameters.polygonMode =
                (uiRessources.bWireFrameMode) ? (VK_POLYGON_MODE_LINE) : (VK_POLYGON_MODE_FILL);
            framebufferResized = true;
        }
        if (ImGui::BeginCombo("##culling", vk_utils::tools::to_string(creationParameters.cullMode).c_str())) {
            for (const auto &cu: cullMode) {
                bool is_selected = (creationParameters.cullMode == cu);
                if (ImGui::Selectable(vk_utils::tools::to_string(cu).c_str(), is_selected)) {
                    creationParameters.cullMode = cu;
                    framebufferResized = true;
                }
                if (is_selected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }
        if (ImGui::BeginCombo("##sample_count", vk_utils::tools::to_string(creationParameters.msaaSample).c_str())) {
            for (const auto &msaa: sampleCount) {
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
        ImGui::Text("");
        ImGui::Text("Camera position");
        ImGui::InputFloat("X", &camera.position.x);
        ImGui::InputFloat("Y", &camera.position.y);
        ImGui::InputFloat("Z", &camera.position.x);
    }
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::End();
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
                        eng->firstMouse = true;
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

    if (eng->firstMouse) {
        eng->lastX = xpos;
        eng->lastY = ypos;
        eng->firstMouse = false;
    }
    auto xoffset = xpos - eng->lastX;
    auto yoffset = eng->lastY - ypos;

    eng->lastX = xpos;
    eng->lastY = ypos;
    eng->camera.processMouseMovement(xoffset, yoffset);
}
