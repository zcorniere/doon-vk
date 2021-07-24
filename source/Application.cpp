#include "Application.hpp"

#include <algorithm>
#include <array>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>
#include <math.h>
#include <memory>
#include <sstream>
#include <stb_image.h>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <tiny_obj_loader.h>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "Camera.hpp"
#include "Logger.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"
#include "types/AllocatedBuffer.hpp"
#include "types/CreationParameters.hpp"
#include "types/Frame.hpp"
#include "types/Mesh.hpp"
#include "types/Vertex.hpp"
#include "types/vk_types.hpp"
#include "vk_init.hpp"
#include "vk_mem_alloc.h"
#include "vk_utils.hpp"

Application::Application(): player()
{
    window.setUserPointer(this);
    window.captureCursor(true);
    window.setKeyCallback(Application::keyboard_callback);
    window.setCursorPosCallback(Application::cursor_callback);
    window.setTitle(uiRessources.sWindowTitle);
}

Application::~Application()
{
    if (device != VK_NULL_HANDLE) vkDeviceWaitIdle(device);
    applicationDeletionQueue.flush();
}

void Application::loadModel()
{
    for (const auto &file: std::filesystem::directory_iterator("../models")) {
        if (file.path().extension() != ".obj") continue;
        logger->info("LOADING") << "Loading object: " << file.path();
        LOGGER_ENDL;

        CPUMesh mesh{};
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file.path().c_str(), nullptr)) {
            if (!warn.empty()) {
                logger->warn("LOADING_OBJ") << warn;
                LOGGER_ENDL;
            }
            if (!err.empty()) {
                logger->err("LOADING_OBJ") << err;
                LOGGER_ENDL;
                throw std::runtime_error("Error while loading obj file");
            }
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto &shape: shapes) {
            for (const auto &index: shape.mesh.indices) {
                Vertex vertex{
                    .pos =
                        {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2],
                        },
                    .normal =
                        {
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2],
                        },

                    .color = {1.0f, 1.0f, 1.0f},
                    .texCoord =
                        {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
                        },
                };

                if (!uniqueVertices.contains(vertex)) {
                    uniqueVertices[vertex] = mesh.verticies.size();
                    mesh.verticies.push_back(vertex);
                }

                mesh.indices.push_back(uniqueVertices.at(vertex));
            }
        }
        loadedMeshes[file.path().stem()] = uploadMesh(mesh);
    }
    applicationDeletionQueue.push([=]() {
        for (auto &[_, i]: loadedMeshes) { vmaDestroyBuffer(allocator, i.meshBuffer.buffer, i.meshBuffer.memory); }
    });
}

void Application::loadTextures()
{
    for (auto &f: std::filesystem::directory_iterator("../textures")) {
        logger->info("LOADING") << "Loading texture: " << f.path();
        LOGGER_ENDL;
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(f.path().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) throw std::runtime_error("failed to load texture image");

        AllocatedBuffer stagingBuffer{};
        stagingBuffer = createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        copyBuffer(stagingBuffer, pixels, imageSize);
        stbi_image_free(pixels);

        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
        AllocatedImage image{};
        VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .extent =
                {
                    .width = static_cast<uint32_t>(texWidth),
                    .height = static_cast<uint32_t>(texHeight),
                    .depth = 1,
                },
            .mipLevels = mipLevels,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        VmaAllocationCreateInfo allocInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        };
        VK_TRY(vmaCreateImage(allocator, &imageInfo, &allocInfo, &image.image, &image.memory, nullptr));
        auto createInfo = vk_init::populateVkImageViewCreateInfo(image.image, VK_FORMAT_R8G8B8A8_SRGB, mipLevels);
        VK_TRY(vkCreateImageView(device, &createInfo, nullptr, &image.imageView));

        transitionImageLayout(image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
        copyBufferToImage(stagingBuffer.buffer, image.image, static_cast<uint32_t>(texWidth),
                          static_cast<uint32_t>(texHeight));
        // transitionImageLayout(image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.memory);
        generateMipmaps(image.image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
        loadedTextures.insert({f.path().stem(), std::move(image)});
    }
    applicationDeletionQueue.push([&]() {
        for (auto &[_, p]: loadedTextures) {
            vkDestroyImageView(device, p.imageView, nullptr);
            vmaDestroyImage(allocator, p.image, p.memory);
        }
    });
}

void Application::run()
{
    float fElapsedTime = 0;

    scene.addObject({
        .meshID = "plane",
        .ubo =
            {
                .transform =
                    {
                        .translation = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, 0.0f, 0.0f)),
                        .rotation = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                        .scale = glm::scale(glm::mat4{1.0f}, glm::vec3(1.0f)),
                    },
                .textureIndex = 3,
            },
    });
    scene.addObject({
        .meshID = "ferdelance",
        .ubo =
            {
                .transform =
                    {
                        .translation = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, 0.0f, 75.0f)),
                        .rotation = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                        .scale = glm::scale(glm::mat4{1.0f}, glm::vec3(0.5f)),
                    },
                .textureIndex = 1,
            },
    });
    scene.addObject({
        .meshID = "cube",
        .ubo =
            {
                .transform =
                    {
                        .translation = glm::translate(glm::mat4{1.0f}, glm::vec3(-10.0f, 2.f, 0.0f)),
                        .rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                        .scale = glm::scale(glm::mat4{1.0f}, glm::vec3(2.0f)),
                    },
                .textureIndex = 0,
            },
    });

    materials.push_back({
        .ambientColor = {1.0f, 1.0f, 1.0f, 1.0f},
        .diffuse = {1.0f, 1.0f, 1.0f, 1.0f},
        .specular = {1.0f, 1.0f, 1.0f, 1.0f},
    });

    void *objectData = nullptr;
    for (auto &frame: frames) {
        vmaMapMemory(allocator, frame.data.materialBuffer.memory, &objectData);
        auto *objectSSBI = (gpuObject::Material *)objectData;
        for (unsigned i = 0; i < materials.size(); i++) { objectSSBI[i] = materials.at(i); }
        vmaUnmapMemory(allocator, frame.data.materialBuffer.memory);
    }
    while (!window.shouldClose()) {
        window.setTitle(uiRessources.sWindowTitle);
        auto tp1 = std::chrono::high_resolution_clock::now();

        window.pollEvent();
        if (!bInteractWithUi) {
            if (window.isKeyPressed(GLFW_KEY_W)) player.processKeyboard(Camera::FORWARD);
            if (window.isKeyPressed(GLFW_KEY_S)) player.processKeyboard(Camera::BACKWARD);
            if (window.isKeyPressed(GLFW_KEY_D)) player.processKeyboard(Camera::RIGHT);
            if (window.isKeyPressed(GLFW_KEY_A)) player.processKeyboard(Camera::LEFT);
            if (window.isKeyPressed(GLFW_KEY_SPACE)) player.processKeyboard(Camera::UP);
            if (window.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) player.processKeyboard(Camera::DOWN);
        }

        drawImgui();
        if (!uiRessources.cameraParamettersOverride.bFlyingCam) {
            player.update(fElapsedTime, -uiRessources.cameraParamettersOverride.fGravity);
        } else {
            player.update(fElapsedTime, 0.0f);
        }
        drawFrame();
        auto tp2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsedTime(tp2 - tp1);
        fElapsedTime = elapsedTime.count();
    }
}

void Application::buildIndirectBuffers(Frame &frame)
{
    void *sceneData = nullptr;
    vmaMapMemory(allocator, frame.indirectBuffer.memory, &sceneData);
    auto *buffer = (VkDrawIndexedIndirectCommand *)sceneData;
    const auto &packedDraws = scene.getDrawBatch();

    for (uint32_t i = 0; i < packedDraws.size(); i++) {
        const auto &mesh = loadedMeshes[packedDraws[i].meshId];

        buffer[i].firstIndex = 0;
        buffer[i].indexCount = mesh.indicesSize;
        buffer[i].vertexOffset = 0;
        buffer[i].instanceCount = 1;
        buffer[i].firstInstance = i;
    }
    vmaUnmapMemory(allocator, frame.indirectBuffer.memory);
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
    clearValues.at(0).color = uiRessources.vClearColor;
    clearValues.at(1).depthStencil = {1.0f, 0};

    void *objectData = nullptr;
    vmaMapMemory(allocator, frame.data.uniformBuffers.memory, &objectData);
    auto *objectSSBI = (gpuObject::UniformBufferObject *)objectData;
    for (unsigned i = 0; i < scene.getNbOfObject(); i++) { objectSSBI[i] = scene.getObject(i).ubo; }
    vmaUnmapMemory(allocator, frame.data.uniformBuffers.memory);
    buildIndirectBuffers(frame);

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
    auto gpuCamera = player.getGPUCameraData(uiRessources.cameraParamettersOverride.fFOV, swapchain.getAspectRatio(),
                                             uiRessources.cameraParamettersOverride.fCloseClippingPlane,
                                             uiRessources.cameraParamettersOverride.fFarClippingPlane);
    VK_TRY(vkBeginCommandBuffer(cmd, &beginInfo));
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frame.data.objectDescriptor, 0,
                            nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &texturesSet, 0, nullptr);
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(gpuCamera), &gpuCamera);
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        for (const auto &draw: scene.getDrawBatch()) {
            const auto &mesh = loadedMeshes[draw.meshId];

            VkBuffer vertexBuffers[] = {mesh.meshBuffer.buffer};
            VkDeviceSize offsets[] = {0};

            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmd, mesh.meshBuffer.buffer, mesh.indicesOffset, VK_INDEX_TYPE_UINT32);

            VkDeviceSize indirectOffset = draw.first * sizeof(VkDrawIndexedIndirectCommand);
            uint32_t draw_stride = sizeof(VkDrawIndexedIndirectCommand);
            vkCmdDrawIndexedIndirect(cmd, frame.indirectBuffer.buffer, indirectOffset, draw.count, draw_stride);
        }
    }
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
        ImGui::ColorEdit4("Clear color", uiRessources.vClearColor.float32);
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
        if (ImGui::Checkbox("Vikin Room ?", &uiRessources.bTmpObject)) {
            if (uiRessources.bTmpObject) {
                scene.addObject({
                    .meshID = "viking_room",
                    .ubo =
                        {
                            .transform =
                                {
                                    .translation = glm::translate(glm::mat4{1.0f}, glm::vec3(-20.0f, 0.f, -20.0f)),
                                    .rotation =
                                        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                                    .scale = glm::scale(glm::mat4{1.0f}, glm::vec3(5.0f)),
                                },
                            .textureIndex = 2,
                        },
                });
            } else {
                scene.removeObject(4);
            }
        }
    }
    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::Text("Position");
        ImGui::InputFloat("X", &player.position.x);
        ImGui::InputFloat("Y", &player.position.y);
        ImGui::InputFloat("Z", &player.position.x);
        ImGui::SliderFloat("FOV", &uiRessources.cameraParamettersOverride.fFOV, 0.f, 180.f);
        ImGui::InputFloat("Close clipping plane", &uiRessources.cameraParamettersOverride.fCloseClippingPlane);
        ImGui::InputFloat("Far clipping plane", &uiRessources.cameraParamettersOverride.fFarClippingPlane);
        ImGui::Checkbox("Flying cam ?", &uiRessources.cameraParamettersOverride.bFlyingCam);
        ImGui::SliderFloat("Gravity", &uiRessources.cameraParamettersOverride.fGravity, 0.0f, 20.f);
        ImGui::InputFloat("Jumping Height", &player.jumpHeight);
    }
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::End();
    ImGui::Render();
}

void Application::keyboard_callback(GLFWwindow *win, int key, int, int action, int)
{
    auto *eng = (Application *)glfwGetWindowUserPointer(win);

    switch (action) {
        case GLFW_PRESS: {
            switch (key) {
                case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(win, true); break;
                case GLFW_KEY_LEFT_ALT: {
                    if (eng->bInteractWithUi) {
                        eng->window.captureCursor(true);
                        eng->bInteractWithUi = false;
                        eng->firstMouse = true;
                    } else {
                        eng->window.captureCursor(false);
                        eng->bInteractWithUi = true;
                    }
                } break;
                case GLFW_KEY_LEFT_CONTROL: {
                    eng->player.movementSpeed *= 2;
                } break;
                default: break;
            }
        } break;
        case GLFW_RELEASE: {
            switch (key) {
                case GLFW_KEY_LEFT_CONTROL: {
                    eng->player.movementSpeed /= 2;
                } break;
                default: break;
            }
        } break;
        default: break;
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
    eng->player.processMouseMovement(xoffset, yoffset);
}
