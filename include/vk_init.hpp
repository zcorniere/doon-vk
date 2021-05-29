#pragma once

#include <vulkan/vulkan.hpp>

namespace vk_init
{
vk::DebugUtilsMessengerCreateInfoEXT populateDebugUtilsMessengerCreateInfoEXT(
    VKAPI_ATTR VkBool32 VKAPI_CALL (*)(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                                       const VkDebugUtilsMessengerCallbackDataEXT *, void *));
}