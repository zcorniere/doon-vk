#pragma once

#include "Camera.hpp"
#include "DeletionQueue.hpp"
#include "QueueFamilyIndices.hpp"
#include "Window.hpp"
#include "types/AllocatedBuffer.hpp"
#include "types/Frame.hpp"
#include "types/Material.hpp"
#include "types/Mesh.hpp"
#include "types/Vertex.hpp"
#include "vk_utils.hpp"

#include <cstring>
#include <stdexcept>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

constexpr uint8_t MAX_FRAME_FRAME_IN_FLIGHT = 2;

class VulkanApplication
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
    void init();
    void recreateSwapchain();
    GPUMesh uploadMesh(const CPUMesh &mesh);

    template <typename T>
    void copyBuffer(AllocatedBuffer &buffer, std::vector<T> data)
    {
        VkDeviceSize size = sizeof(data[0]) * data.size();
        void *mapped = nullptr;
        vmaMapMemory(allocator, buffer.memory, &mapped);
        std::memcpy(mapped, data.data(), size);
        vmaUnmapMemory(allocator, buffer.memory);
    }

    template <typename T>
    void copyBuffer(AllocatedBuffer &buffer, T *data, size_t size)
    {
        void *mapped = nullptr;
        vmaMapMemory(allocator, buffer.memory, &mapped);
        std::memcpy(mapped, data, size);
        vmaUnmapMemory(allocator, buffer.memory);
    }
    bool framebufferResized = false;

private:
    static std::vector<const char *> getRequiredExtensions(bool bEnableValidationLayers = false);
    static bool checkValiationLayerSupport();
    static uint32_t debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                                  const VkDebugUtilsMessengerCallbackDataEXT *, void *);
    static bool isDeviceSuitable(const VkPhysicalDevice &gpu, const VkSurfaceKHR &surface);
    static bool checkDeviceExtensionSupport(const VkPhysicalDevice &device);
    static void framebufferResizeCallback(GLFWwindow *, int, int);
    static VkSampleCountFlagBits getMexUsableSampleCount(VkPhysicalDevice &physical_device);

private:
    void initInstance();
    void initDebug();
    void pickPhysicalDevice();
    void initSurface();
    void createLogicalDevice();
    void createAllocator();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void loadModel();
    void createDescriptorSetLayout();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void loadTextures();
    void createTextureSampler();
    void createDepthResources();
    void copyBuffer(const VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize &size);
    void copyBufferToImage(const VkBuffer &srcBuffer, VkImage &dstBuffer, uint32_t width, uint32_t height);
    void immediateCommand(std::function<void(VkCommandBuffer &)> &&);
    void transitionImageLayout(VkImage &image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                               uint32_t mipLevels = 1);
    void createColorResources();
    void generateMipmaps(VkImage &image, VkFormat imageFormat, uint32_t texWidth, uint32_t texHeight,
                         uint32_t mipLevel);

protected:
    Window window;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkSampleCountFlagBits msaaSample = VK_SAMPLE_COUNT_1_BIT;
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator;

    //  Queues
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    // Surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    // Swapchain
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    // Pipeline
    VkRenderPass renderPass;
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

    // Uniform buffers
    std::vector<AllocatedBuffer> uniformBuffers;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    // Texture
    std::unordered_map<std::string, AllocatedImage> loadedTextures;
    uint32_t mipLevels = 0;
    VkSampler textureSampler = VK_NULL_HANDLE;

    // Depthbuffer
    AllocatedImage depthResources = {};

    AllocatedImage colorImage = {};

private:
    DeleteionQueue mainDeletionQueue;
    DeleteionQueue swapchainDeletionQueue;
};
