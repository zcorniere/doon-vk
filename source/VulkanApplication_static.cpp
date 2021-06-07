#include "Logger.hpp"
#include "SwapChainSupportDetails.hpp"
#include "VulkanApplication.hpp"
#include <cstring>
#include <set>
#include <string>

std::vector<const char *> VulkanApplication::getRequiredExtensions(bool bEnableValidationLayers)
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtentsions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtentsions, glfwExtentsions + glfwExtensionCount);

    if (bEnableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }
    return extensions;
}

bool VulkanApplication::checkValiationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName: validationLayers) {
        bool layerFound = false;

        for (const auto &layerProperties: availableLayers) {
            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) return false;
    }

    return true;
}

static const char *to_string_message_type(VkDebugUtilsMessageTypeFlagsEXT s)
{
    if (s == 7) return "General | Validation | Performance";
    if (s == 6) return "Validation | Performance";
    if (s == 5) return "General | Performance";
    if (s == 4 /*VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT*/) return "Performance";
    if (s == 3) return "General | Validation";
    if (s == 2 /*VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT*/) return "Validation";
    if (s == 1 /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT*/) return "General";
    return "Unknown";
}

uint32_t VulkanApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                          VkDebugUtilsMessageTypeFlagsEXT messageType,
                                          const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
{
    std::stringstream &(Logger::*severity)(const std::string &) = nullptr;
    switch (messageSeverity) {
        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            severity = &Logger::debug;
            break;
        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            severity = &Logger::err;
            break;
        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            severity = &Logger::warn;
            break;
        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            severity = &Logger::info;
            break;
        default: severity = &Logger::err; break;
    }
    (logger->*severity)(to_string_message_type(messageType)) << pCallbackData->pMessage;
    logger->endl();
    return VK_FALSE;
}

bool VulkanApplication::isDeviceSuitable(const VkPhysicalDevice &gpu, const VkSurfaceKHR &surface)
{
    auto indices = QueueFamilyIndices::findQueueFamilies(gpu, surface);
    bool extensionsSupported = checkDeviceExtensionSupport(gpu);
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(gpu, &deviceProperties);
    vkGetPhysicalDeviceFeatures(gpu, &deviceFeatures);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        auto swapChainSupport = SwapChainSupportDetails::querySwapChainSupport(gpu, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool VulkanApplication::checkDeviceExtensionSupport(const VkPhysicalDevice &device)
{
    uint32_t extensionsCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto &extension: availableExtensions) { requiredExtensions.erase(extension.extensionName); }
    return requiredExtensions.empty();
}
void VulkanApplication::framebufferResizeCallback(GLFWwindow *window, int, int)
{
    auto app = reinterpret_cast<VulkanApplication *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}