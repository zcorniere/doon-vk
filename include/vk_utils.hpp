#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan.h>

namespace vk_utils
{
namespace tools
{
    std::string errorString(VkResult errorCode);
    std::string physicalDeviceTypeString(VkPhysicalDeviceType type);
}    // namespace tools
}    // namespace vk_utils

struct VulkanException : public std::runtime_error {
    VulkanException(const std::string message): std::runtime_error(message) {}
    VulkanException(const VkResult er): std::runtime_error(vk_utils::tools::errorString(er)) {}
};

#define VK_TRY(x)                                \
    {                                            \
        VkResult err = x;                        \
        if (err) { throw VulkanException(err); } \
    }
