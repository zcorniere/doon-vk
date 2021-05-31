#pragma once

#include <vulkan/vulkan.hpp>

namespace vk_init
{
VkInstanceCreateInfo populateVkInstanceCreateInfo(VkApplicationInfo &appCreateInfo,
                                                  const std::vector<const char *> &vExtentions,
                                                  const std::vector<const char *> &vLayers);
VkDebugUtilsMessengerCreateInfoEXT populateDebugUtilsMessengerCreateInfoEXT(
    VKAPI_ATTR VkBool32 VKAPI_CALL (*)(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                                       const VkDebugUtilsMessengerCallbackDataEXT *, void *));
VkDeviceQueueCreateInfo populateDeviceQueueCreateInfo(const uint32_t, const uint32_t, const float &);
VkImageViewCreateInfo populateVkImageViewCreateInfo(VkImage &image, VkFormat &format);
}    // namespace vk_init