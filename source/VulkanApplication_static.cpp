#include "Logger.hpp"
#include "QueueFamilyIndices.hpp"
#include "VulkanApplication.hpp"

std::vector<const char *> VulkanApplication::getRequiredExtensions(bool bEnableValidationLayers)
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (bEnableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }

    return extensions;
}

bool VulkanApplication::checkValiationLayerSupport()
{
    auto availableLayers = vk::enumerateInstanceLayerProperties();
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

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

bool VulkanApplication::isDeviceSuitable(const vk::PhysicalDevice &gpu)
{
    auto indices = QueueFamilyIndices::findQueueFamilies(gpu);
    auto feature = gpu.getFeatures();
    auto properties = gpu.getProperties();
    return indices.isComplete();
}