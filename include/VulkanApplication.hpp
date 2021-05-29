#pragma once

#include "Window.hpp"
#include <memory>

#include <vulkan/vulkan.hpp>

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
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
    vk::PhysicalDevice physical_device;
    vk::Device device;
    vk::Queue graphicsQueue;

private:
    static std::vector<const char *> getRequiredExtensions(bool bEnableValidationLayers = false);
    static bool checkValiationLayerSupport();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                        VkDebugUtilsMessageTypeFlagsEXT,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *, void *);
    static bool isDeviceSuitable(const vk::PhysicalDevice &gpu);

private:
    void pickPhysicalDevice();
    void createLogicalDevice();

private:
};
