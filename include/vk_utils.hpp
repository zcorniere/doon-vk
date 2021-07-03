#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vk_utils
{
std::vector<std::byte> readFile(const std::string &filename);
VkShaderModule createShaderModule(const VkDevice &device, const std::vector<std::byte> &code);
VkFormat findSupportedFormat(VkPhysicalDevice &gpu, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features);
bool hasStencilComponent(VkFormat format);
uint32_t findMemoryType(VkPhysicalDevice &physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

namespace tools
{
    const std::string to_string(VkSampleCountFlagBits count);
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