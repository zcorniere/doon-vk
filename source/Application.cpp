#include "Application.hpp"
#include "Logger.hpp"
#include "vk_init.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>

Application::Application(): camera(glm::vec3(0.0f, 2.0f, 0.0f))
{
    window.setUserPointer(this);
    window.captureCursor(true);
    window.setKeyCallback(Application::keyboard_callback);
    window.setCursorPosCallback(Application::cursor_callback);
    window.setTitle(uiRessources.sWindowTitle);
    this->VulkanApplication::init();
}

Application::~Application()
{
    vkDeviceWaitIdle(device);
    applicationDeletionQueue.flush();
}

void Application::loadModel()
{
    for (auto &file: std::filesystem::directory_iterator("../models")) {
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
                    .color = {1.0f, 1.0f, 1.0f},
                    .texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                                 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]},
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

    sceneModels.push_back({
        .meshID = "plane",
        .ubo =
            {
                .transform =
                    {
                        .translation = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, 0.0f, 0.0f)),
                        .rotation = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                        .scale = glm::scale(glm::mat4{1.0f}, glm::vec3(1.0f)),
                    },
                .textureIndex = 2,
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
    auto gpuCamera = camera.getGPUCameraData(uiRessources.cameraParamettersOverride.fFOV,
                                             uiRessources.cameraParamettersOverride.fAspectRatio[0] /
                                                 uiRessources.cameraParamettersOverride.fAspectRatio[1],
                                             uiRessources.cameraParamettersOverride.fCloseClippingPlane,
                                             uiRessources.cameraParamettersOverride.fFarClippingPlane);
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
    }
    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::Text("Position");
        ImGui::InputFloat("X", &camera.position.x);
        ImGui::InputFloat("Y", &camera.position.y);
        ImGui::InputFloat("Z", &camera.position.x);
        ImGui::SliderFloat("FOV", &uiRessources.cameraParamettersOverride.fFOV, 0.f, 180.f);
        ImGui::InputFloat("Close clipping plane", &uiRessources.cameraParamettersOverride.fCloseClippingPlane);
        ImGui::InputFloat("Far clipping plane", &uiRessources.cameraParamettersOverride.fFarClippingPlane);
        ImGui::Text("Aspect ratio");
        ImGui::SliderFloat2("", uiRessources.cameraParamettersOverride.fAspectRatio, 0.f, 2048.f, "", 1.f);
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
