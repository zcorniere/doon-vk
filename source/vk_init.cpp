#include "vk_init.hpp"

namespace vk_init
{
vk::DebugUtilsMessengerCreateInfoEXT populateDebugUtilsMessengerCreateInfoEXT(VKAPI_ATTR VkBool32 VKAPI_CALL (
    *debugMessageFunc)(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                       const VkDebugUtilsMessengerCallbackDataEXT *, void *))
{

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugInfo = {
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = debugMessageFunc,
    };
    return debugInfo;
}

}    // namespace vk_init