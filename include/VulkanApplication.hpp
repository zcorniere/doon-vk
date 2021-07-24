#pragma once

#include <cstring>
#include <functional>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "DeletionQueue.hpp"
#include "QueueFamilyIndices.hpp"
#include "Swapchain.hpp"
#include "VulkanLoader.hpp"
#include "Window.hpp"
#include "types/AllocatedBuffer.hpp"
#include "types/CreationParameters.hpp"
#include "types/Frame.hpp"
#include "types/Material.hpp"
#include "types/Mesh.hpp"
#include "types/Vertex.hpp"
#include "types/vk_types.hpp"
#include "vk_utils.hpp"

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

constexpr uint8_t MAX_FRAME_FRAME_IN_FLIGHT = 2;

class VulkanApplication : public VulkanLoader
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
    AllocatedBuffer createBuffer(uint32_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

protected:
    template <typename T>
    void copyBuffer(AllocatedBuffer &buffer, std::vector<T> data);
    template <typename T>
    void copyBuffer(AllocatedBuffer &buffer, T *data, size_t size);
    void copyBuffer(const VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize &size);
    GPUMesh uploadMesh(const CPUMesh &mesh);
    void immediateCommand(std::function<void(VkCommandBuffer &)> &&);
    void copyBufferToImage(const VkBuffer &srcBuffer, VkImage &dstBuffer, uint32_t width, uint32_t height);
    void transitionImageLayout(VkImage &image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                               uint32_t mipLevels = 1);
    void generateMipmaps(VkImage &image, VkFormat imageFormat, uint32_t texWidth, uint32_t texHeight,
                         uint32_t mipLevel);

private:
    static bool checkValiationLayerSupport();
    static std::vector<const char *> getRequiredExtensions(bool bEnableValidationLayers = false);
    static uint32_t debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                                  const VkDebugUtilsMessengerCallbackDataEXT *, void *);
    static void framebufferResizeCallback(GLFWwindow *, int, int);
    static VkSampleCountFlagBits getMexUsableSampleCount(VkPhysicalDevice &physical_device);

private:
    bool isDeviceSuitable(const VkPhysicalDevice &gpu, const VkSurfaceKHR &surface);
    bool checkDeviceExtensionSupport(const VkPhysicalDevice &device);

    void initInstance();
    void initDebug();
    void pickPhysicalDevice();
    void initSurface();
    void createLogicalDevice();
    void createAllocator();
    void createRenderPass();
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
    VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkSampleCountFlagBits maxMsaaSample = VK_SAMPLE_COUNT_1_BIT;
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator;

    //  Queues
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    // Surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    // Swapchain
    Swapchain swapchain;

    // Pipeline
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    // Framebuffer
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Command Pool
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    // Instant commandbuffers
    struct {
        VkFence uploadFence = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
    } uploadContext = {};

    // Sync
    uint8_t currentFrame = 0;
    Frame frames[MAX_FRAME_FRAME_IN_FLIGHT];

    std::unordered_map<std::string, GPUMesh> loadedMeshes;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    // Texture
    std::unordered_map<std::string, AllocatedImage> loadedTextures;
    uint32_t mipLevels = 0;
    VkSampler textureSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout texturesSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet texturesSet = VK_NULL_HANDLE;

    // Depthbuffer
    AllocatedImage depthResources = {};
    AllocatedImage colorImage = {};

private:
    DeletionQueue mainDeletionQueue;
    DeletionQueue swapchainDeletionQueue;
};

template <typename T>
void VulkanApplication::copyBuffer(AllocatedBuffer &buffer, T *data, size_t size)
{
    void *mapped = nullptr;
    vmaMapMemory(allocator, buffer.memory, &mapped);
    std::memcpy(mapped, data, size);
    vmaUnmapMemory(allocator, buffer.memory);
}

template <typename T>
void VulkanApplication::copyBuffer(AllocatedBuffer &buffer, std::vector<T> data)
{
    VkDeviceSize size = sizeof(data[0]) * data.size();
    void *mapped = nullptr;
    vmaMapMemory(allocator, buffer.memory, &mapped);
    std::memcpy(mapped, data.data(), size);
    vmaUnmapMemory(allocator, buffer.memory);
}
