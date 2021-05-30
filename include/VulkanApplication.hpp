#pragma once

#include "DeletionQueue.hpp"
#include "vk_utils.hpp"

#include "Window.hpp"
#include <vector>

#include <stdexcept>
#include <vulkan/vulkan.h>

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

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
    void init(Window &);

protected:
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

private:
    static std::vector<const char *> getRequiredExtensions(bool bEnableValidationLayers = false);
    static bool checkValiationLayerSupport();
    static uint32_t debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                                  const VkDebugUtilsMessengerCallbackDataEXT *, void *);
    static bool isDeviceSuitable(const VkPhysicalDevice &gpu, const VkSurfaceKHR &surface);

private:
    void initInstance();
    void initDebug();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void initSurface(Window &);

private:
    DeleteionQueue mainDeletionQueue;
};

struct VulkanException : public std::runtime_error {
    VulkanException(const std::string message): std::runtime_error(message) {}
    VulkanException(const VkResult er): std::runtime_error(vk_utils::tools::errorString(er)) {}
};

#define VK_TRY(x)                                \
    {                                            \
        VkResult err = x;                        \
        if (err) { throw VulkanException(err); } \
    }
