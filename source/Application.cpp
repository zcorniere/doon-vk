#include "Application.hpp"

#include <Logger.hpp>
#include <ProgressBar.hpp>
#include <algorithm>
#include <array>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
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
#include <vk_mem_alloc.hpp>

#include "Camera.hpp"
#include "DebugMacros.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"
#include "types/AllocatedBuffer.hpp"
#include "types/CreationParameters.hpp"
#include "types/Frame.hpp"
#include "types/Mesh.hpp"
#include "types/Vertex.hpp"
#include "types/vk_types.hpp"
#include "vk_init.hpp"
#include "vk_utils.hpp"

Application::Application(): player()
{
    DEBUG_FUNCTION
    window.setUserPointer(this);
    window.captureCursor(true);
    window.setKeyCallback(Application::keyboard_callback);
    window.setCursorPosCallback(Application::cursor_callback);
    window.setTitle(uiRessources.sWindowTitle);
}

Application::~Application()
{
    DEBUG_FUNCTION
    if (device) vkDeviceWaitIdle(device);
    applicationDeletionQueue.flush();
}

void Application::loadModel()
{
    DEBUG_FUNCTION
    std::vector<Vertex> vertexStagingBuffer;
    std::vector<uint32_t> indexStagingBuffer;

    auto iterator = std::filesystem::directory_iterator("../models");
    auto distance = std::distance(begin(iterator), {});

    auto &bar = logger->newProgressBar("Model", distance);
    for (const auto &file: std::filesystem::directory_iterator("../models")) {
        ++bar;
        if (file.path().extension() != ".obj") { continue; }
        logger->info("LOADING") << "Loading object: " << file.path();
        LOGGER_ENDL;

        GPUMesh mesh{
            .verticiesOffset = vertexStagingBuffer.size(),
            .indicesOffset = indexStagingBuffer.size(),
        };
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file.path().c_str(), nullptr);
        if (!warn.empty()) {
            logger->warn("LOADING_OBJ") << warn;
            LOGGER_ENDL;
        }
        if (!err.empty()) {
            logger->err("LOADING_OBJ") << err;
            LOGGER_ENDL;
            throw std::runtime_error("Error while loading obj file");
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
                };
                if (!attrib.normals.empty()) {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],
                    };
                }
                if (!attrib.texcoords.empty()) {
                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
                    };
                }

                if (!uniqueVertices.contains(vertex)) {
                    uniqueVertices[vertex] = vertexStagingBuffer.size() - mesh.verticiesOffset;
                    vertexStagingBuffer.push_back(vertex);
                }
                indexStagingBuffer.push_back(uniqueVertices.at(vertex));
            }
        }
        mesh.indicesSize = indexStagingBuffer.size() - mesh.indicesOffset;
        mesh.verticiesSize = vertexStagingBuffer.size() - mesh.verticiesOffset;
        loadedMeshes[file.path().stem()] = mesh;
    }
    auto vertexSize = vertexStagingBuffer.size() * sizeof(Vertex);
    auto stagingVertex = createBuffer(vertexSize, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuToGpu);
    vertexBuffers =
        createBuffer(vertexSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                     vma::MemoryUsage::eGpuOnly);
    copyBuffer(stagingVertex, vertexStagingBuffer);
    copyBufferToBuffer(stagingVertex.buffer, vertexBuffers.buffer, vertexSize);

    auto indexSize = indexStagingBuffer.size() * sizeof(uint32_t);
    auto stagingIndex = createBuffer(indexSize, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuToGpu);
    indicesBuffers =
        createBuffer(indexSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                     vma::MemoryUsage::eGpuOnly);
    copyBuffer(stagingIndex, indexStagingBuffer);
    copyBufferToBuffer(stagingIndex.buffer, indicesBuffers.buffer, indexSize);

    vmaDestroyBuffer(allocator, stagingVertex.buffer, stagingVertex.memory);
    vmaDestroyBuffer(allocator, stagingIndex.buffer, stagingIndex.memory);
    logger->deleteProgressBar(bar);
    applicationDeletionQueue.push([&] {
        vmaDestroyBuffer(allocator, vertexBuffers.buffer, vertexBuffers.memory);
        vmaDestroyBuffer(allocator, indicesBuffers.buffer, indicesBuffers.memory);
    });
}

void Application::loadTextures()
{
    DEBUG_FUNCTION
    auto iterator = std::filesystem::directory_iterator("../textures");
    auto distance = std::distance(begin(iterator), end(iterator));

    auto &bar = logger->newProgressBar("Texture", distance);
    for (auto &f: std::filesystem::directory_iterator("../textures")) {
        ++bar;

        logger->info("LOADING") << "Loading texture: " << f.path();
        LOGGER_ENDL;
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(f.path().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        vk::DeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) throw std::runtime_error("failed to load texture image");

        AllocatedBuffer stagingBuffer{};
        stagingBuffer = createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuToGpu);
        copyBuffer(stagingBuffer, pixels, imageSize);
        stbi_image_free(pixels);

        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
        AllocatedImage image{};
        vk::ImageCreateInfo imageInfo{
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Srgb,
            .extent =
                {
                    .width = static_cast<uint32_t>(texWidth),
                    .height = static_cast<uint32_t>(texHeight),
                    .depth = 1,
                },
            .mipLevels = mipLevels,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eSampled,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined,
        };
        vma::AllocationCreateInfo allocInfo;
        allocInfo.usage = vma::MemoryUsage::eGpuOnly;

        std::tie(image.image, image.memory) = allocator.createImage(imageInfo, allocInfo);

        auto createInfo = vk_init::populateVkImageViewCreateInfo(image.image, vk::Format::eR8G8B8A8Srgb, mipLevels);
        image.imageView = device.createImageView(createInfo);

        transitionImageLayout(image.image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined,
                              vk::ImageLayout::eTransferDstOptimal, mipLevels);
        copyBufferToImage(stagingBuffer.buffer, image.image, static_cast<uint32_t>(texWidth),
                          static_cast<uint32_t>(texHeight));
        // transitionImageLayout(image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        allocator.destroyBuffer(stagingBuffer.buffer, stagingBuffer.memory);
        generateMipmaps(image.image, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels);
        loadedTextures.insert({f.path().stem(), std::move(image)});
    }
    logger->deleteProgressBar(bar);
    applicationDeletionQueue.push([&] {
        for (auto &[_, p]: loadedTextures) {
            device.destroy(p.imageView);
            allocator.destroyImage(p.image, p.memory);
        }
    });
}

void Application::run()
{
    DEBUG_FUNCTION;
    float fElapsedTime = 0;

    scene.addObject({
        .meshID = "plane",
        .ubo =
            {
                .transform =
                    {
                        .translation = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, 0.0f, 0.0f)),
                        .rotation = glm::toMat4(glm::quat(glm::vec3(0, 0, 0))),
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
                        .rotation = glm::toMat4(glm::quat(glm::vec3(0, 0, 0))),
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
                        .rotation = glm::toMat4(glm::quat(glm::vec3(-(M_PI / 2), 0, 0))),
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
        player.isFreeFly = uiRessources.cameraParamettersOverride.bFlyingCam;
        player.update(fElapsedTime, -uiRessources.cameraParamettersOverride.fGravity);
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
    auto *buffer = (vk::DrawIndexedIndirectCommand *)sceneData;
    const auto &packedDraws = scene.getDrawBatch();

    for (uint32_t i = 0; i < packedDraws.size(); i++) {
        const auto &mesh = loadedMeshes.at(packedDraws[i].meshId);

        buffer[i].firstIndex = mesh.indicesOffset;
        buffer[i].indexCount = mesh.indicesSize;
        buffer[i].vertexOffset = mesh.verticiesOffset;
        buffer[i].instanceCount = 1;
        buffer[i].firstInstance = i;
    }
    vmaUnmapMemory(allocator, frame.indirectBuffer.memory);
}

void Application::drawFrame()
{
    auto &frame = frames[currentFrame];
    uint32_t imageIndex;
    vk::Result result;

    VK_TRY(device.waitForFences(frame.inFlightFences, VK_TRUE, UINT64_MAX));
    std::tie(result, imageIndex) =
        device.acquireNextImageKHR(swapchain.getSwapchain(), UINT64_MAX, frame.imageAvailableSemaphore);

    if (vk_utils::isSwapchainInvalid(result, vk::Result::eErrorOutOfDateKHR)) { return recreateSwapchain(); }

    auto &cmd = commandBuffers[imageIndex];

    device.resetFences(frame.inFlightFences);
    cmd.reset();

    vk::Semaphore waitSemaphores[] = {frame.imageAvailableSemaphore};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::Semaphore signalSemaphores[] = {frame.renderFinishedSemaphore};

    vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };

    std::array<vk::ClearValue, 2> clearValues{};
    clearValues.at(0).color = vk::ClearColorValue{uiRessources.vClearColor};
    clearValues.at(1).depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

    void *objectData = nullptr;
    vmaMapMemory(allocator, frame.data.uniformBuffers.memory, &objectData);
    auto *objectSSBI = (gpuObject::UniformBufferObject *)objectData;
    for (unsigned i = 0; i < scene.getNbOfObject(); i++) { objectSSBI[i] = scene.getObject(i).ubo; }
    vmaUnmapMemory(allocator, frame.data.uniformBuffers.memory);
    buildIndirectBuffers(frame);

    vk::RenderPassBeginInfo renderPassInfo{
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

    vk::CommandBufferBeginInfo beginInfo{
        .pInheritanceInfo = nullptr,
    };
    VK_TRY(cmd.begin(&beginInfo));
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, frame.data.objectDescriptor, nullptr);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, texturesSet, nullptr);
    cmd.pushConstants<Camera::GPUCameraData>(
        pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, gpuCamera);

    vk::DeviceSize offset = 0;
    cmd.bindVertexBuffers(0, vertexBuffers.buffer, offset);
    cmd.bindIndexBuffer(indicesBuffers.buffer, 0, vk::IndexType::eUint32);
    cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    {
        for (const auto &draw: scene.getDrawBatch()) {
            cmd.drawIndexedIndirect(frame.indirectBuffer.buffer, draw.first * sizeof(vk::DrawIndexedIndirectCommand),
                                    draw.count, sizeof(vk::DrawIndexedIndirectCommand));
        }
    }
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    cmd.endRenderPass();
    cmd.end();
    graphicsQueue.submit(submitInfo, frame.inFlightFences);

    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = &(swapchain.getSwapchain()),
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };

    result = presentQueue.presentKHR(presentInfo);
    if (vk_utils::isSwapchainInvalid(result, vk::Result::eErrorOutOfDateKHR, vk::Result::eSuboptimalKHR) ||
        framebufferResized) {
        framebufferResized = false;
        return recreateSwapchain();
    }

    currentFrame = (currentFrame + 1) % MAX_FRAME_FRAME_IN_FLIGHT;
}

void Application::drawImgui()
{
    static const std::vector<vk::SampleCountFlagBits> sampleCount = {
        vk::SampleCountFlagBits::e1,  vk::SampleCountFlagBits::e2,  vk::SampleCountFlagBits::e4,
        vk::SampleCountFlagBits::e8,  vk::SampleCountFlagBits::e16, vk::SampleCountFlagBits::e32,
        vk::SampleCountFlagBits::e64,
    };
    static const std::vector<vk::CullModeFlagBits> cullMode = {
        vk::CullModeFlagBits::eNone,
        vk::CullModeFlagBits::eBack,
        vk::CullModeFlagBits::eFront,
        vk::CullModeFlagBits::eFrontAndBack,
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
        ImGui::ColorEdit4("Clear color", uiRessources.vClearColor.data());
    }

    if (ImGui::CollapsingHeader("Render")) {
        if (ImGui::Button("Recreate Swapchain")) { framebufferResized = true; }
        if (ImGui::Checkbox("Wireframe mode", &uiRessources.bWireFrameMode)) {
            creationParameters.polygonMode =
                (uiRessources.bWireFrameMode) ? (vk::PolygonMode::eLine) : (vk::PolygonMode::eFill);
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
                                    .rotation = glm::toMat4(glm::quat(glm::vec3(0, -(M_PI / 2), 0))),
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
