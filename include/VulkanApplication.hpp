#pragma once

#include <cstring>
#include <functional>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

#include "DeletionQueue.hpp"
#include "Swapchain.hpp"
#include "VulkanLoader.hpp"
#include "Window.hpp"
#include "types/AllocatedBuffer.hpp"
#include "types/CreationParameters.hpp"
#include "types/Frame.hpp"
#include "types/Mesh.hpp"
#include "vk_utils.hpp"

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

constexpr uint8_t MAX_FRAME_FRAME_IN_FLIGHT = 3;

class VulkanApplication : protected VulkanLoader
{
public:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

public:
    VulkanApplication();
    ~VulkanApplication();

    void init(std::function<bool()> &&loadingStage);
    void recreateSwapchain();
    AllocatedBuffer createBuffer(uint32_t allocSize, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage);

protected:
    template <vk_utils::is_copyable T>
    void copyBuffer(AllocatedBuffer &buffer, const std::vector<T> &data);
    template <vk_utils::is_copyable T>
    void copyBuffer(AllocatedBuffer &buffer, const T *data, const size_t size);

    GPUMesh uploadMesh(const CPUMesh &mesh);
    void immediateCommand(std::function<void(vk::CommandBuffer &)> &&);
    void copyBufferToImage(const vk::Buffer &srcBuffer, vk::Image &dstBuffer, uint32_t width, uint32_t height);
    void copyBufferToBuffer(const vk::Buffer &srcBuffer, vk::Buffer &dstBuffer, const vk::DeviceSize &size);

    void transitionImageLayout(vk::Image &image, vk::Format format, vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout, uint32_t mipLevels = 1);
    void generateMipmaps(vk::Image &image, vk::Format imageFormat, uint32_t texWidth, uint32_t texHeight,
                         uint32_t mipLevel);

private:
    static bool checkValiationLayerSupport();
    static uint32_t debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                                  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *);
    static void framebufferResizeCallback(GLFWwindow *, int, int);
    static vk::SampleCountFlagBits getMexUsableSampleCount(vk::PhysicalDevice &physical_device);
    static bool isDeviceSuitable(const vk::PhysicalDevice &gpu, const vk::SurfaceKHR &surface);
    static uint32_t rateDeviceSuitability(const vk::PhysicalDevice &device);
    static bool checkDeviceExtensionSupport(const vk::PhysicalDevice &device);

private:
    void initInstance();
    void initDebug();
    void pickPhysicalDevice();
    void initSurface();
    void createLogicalDevice();
    void createAllocator();
    void createRenderPass();

    void createPipelineCache();
    void createPipelineLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void createDescriptorSetLayout();
    void createTextureDescriptorSetLayout();
    void createUniformBuffers();
    void createIndirectBuffer();
    void createDescriptorPool();
    void createDescriptorSets();
    void createTextureDescriptorSets();
    void createTextureSampler();
    void createDepthResources();
    void createColorResources();
    void createImgui();

public:
    bool framebufferResized = false;

protected:
    CreationParameters creationParameters = {};
    Window window;
    vk::DebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
    vk::PhysicalDevice physical_device = VK_NULL_HANDLE;
    vk::SampleCountFlagBits maxMsaaSample = vk::SampleCountFlagBits::e1;
    vma::Allocator allocator;

    //  Queues
    vk::Queue graphicsQueue = VK_NULL_HANDLE;
    vk::Queue presentQueue = VK_NULL_HANDLE;

    // Surface
    vk::SurfaceKHR surface = VK_NULL_HANDLE;

    // Swapchain
    Swapchain swapchain;

    // Pipeline
    vk::RenderPass renderPass = VK_NULL_HANDLE;
    vk::DescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    vk::PipelineLayout pipelineLayout = VK_NULL_HANDLE;
    vk::Pipeline graphicsPipeline = VK_NULL_HANDLE;
    vk::PipelineCache pipelineCache = VK_NULL_HANDLE;

    // Framebuffer
    std::vector<vk::Framebuffer> swapChainFramebuffers;

    // Command Pool
    vk::CommandPool commandPool = VK_NULL_HANDLE;
    std::vector<vk::CommandBuffer> commandBuffers;

    // Instant commandbuffers
    struct {
        vk::Fence uploadFence = VK_NULL_HANDLE;
        vk::CommandPool commandPool = VK_NULL_HANDLE;
    } uploadContext = {};

    // Sync
    uint8_t currentFrame = 0;
    Frame frames[MAX_FRAME_FRAME_IN_FLIGHT];

    // Models
    AllocatedBuffer vertexBuffers;
    AllocatedBuffer indicesBuffers;
    std::unordered_map<std::string, GPUMesh> loadedMeshes;

    vk::DescriptorPool descriptorPool = VK_NULL_HANDLE;

    // Texture
    std::unordered_map<std::string, AllocatedImage> loadedTextures;
    uint32_t mipLevels = 0;
    vk::Sampler textureSampler = VK_NULL_HANDLE;
    vk::DescriptorSetLayout texturesSetLayout = VK_NULL_HANDLE;
    vk::DescriptorSet texturesSet = VK_NULL_HANDLE;

    // Depthbuffer
    AllocatedImage depthResources = {};
    AllocatedImage colorImage = {};

private:
    DeletionQueue mainDeletionQueue;
    DeletionQueue swapchainDeletionQueue;
};

#ifndef VULKAN_APPLICATION_IMPLEMENTATION
#define VULKAN_APPLICATION_IMPLEMENTATION

template <vk_utils::is_copyable T>
void VulkanApplication::copyBuffer(AllocatedBuffer &buffer, const T *data, const size_t size)
{
    void *mapped = allocator.mapMemory(buffer.memory);
    std::memcpy(mapped, data, size);
    allocator.unmapMemory(buffer.memory);
}

template <vk_utils::is_copyable T>
void VulkanApplication::copyBuffer(AllocatedBuffer &buffer, const std::vector<T> &data)
{
    vk::DeviceSize size = sizeof(data[0]) * data.size();

    void *mapped = allocator.mapMemory(buffer.memory);
    std::memcpy(mapped, data.data(), size);
    allocator.unmapMemory(buffer.memory);
}

#endif
