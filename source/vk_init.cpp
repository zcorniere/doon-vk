#include "vk_init.hpp"

namespace vk_init
{
VkDebugUtilsMessengerCreateInfoEXT populateDebugUtilsMessengerCreateInfoEXT(VKAPI_ATTR VkBool32 VKAPI_CALL (
    *debugMessageFunc)(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                       const VkDebugUtilsMessengerCallbackDataEXT *, void *))
{

    VkDebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    VkDebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    return VkDebugUtilsMessengerCreateInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = debugMessageFunc,
    };
}

VkDeviceQueueCreateInfo populateDeviceQueueCreateInfo(const uint32_t count, const uint32_t queue, const float &priority)
{
    return VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .queueFamilyIndex = queue,
        .queueCount = count,
        .pQueuePriorities = &priority,
    };
}

}    // namespace vk_init