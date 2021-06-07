#pragma once

#include "DeletionQueue.hpp"
#include "Frame.hpp"
#include "QueueFamilyIndices.hpp"
#include "Window.hpp"
#include "vk_utils.hpp"

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
constexpr uint8_t MAX_FRAME_FRAME_IN_FLIGHT = 2;

class VulkanApplication
{
public:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

protected:
    VulkanApplication();
    ~VulkanApplication();
    void init();

protected:
    Window window;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

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
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    // Framebuffer
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Command Pool
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // Sync
    uint8_t currentFrame = 0;
    Frame frames[MAX_FRAME_FRAME_IN_FLIGHT];

private:
    static std::vector<const char *> getRequiredExtensions(bool bEnableValidationLayers = false);
    static bool checkValiationLayerSupport();
    static uint32_t debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                                  const VkDebugUtilsMessengerCallbackDataEXT *, void *);
    static bool isDeviceSuitable(const VkPhysicalDevice &gpu, const VkSurfaceKHR &surface);
    static bool checkDeviceExtensionSupport(const VkPhysicalDevice &device);

private:
    void initInstance();
    void initDebug();
    void pickPhysicalDevice();
    void initSurface();
    void createLogicalDevice();
    void createSwapchain();
    void createImageWiews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

private:
    DeleteionQueue mainDeletionQueue;
};
