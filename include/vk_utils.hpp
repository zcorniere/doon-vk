#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "types/VulkanException.hpp"

#define VK_TRY(x)                                \
    {                                            \
        VkResult err = x;                        \
        if (err) { throw VulkanException(err); } \
    }

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
    const std::string to_string(VkCullModeFlags count);

    std::string physicalDeviceTypeString(VkPhysicalDeviceType type);
}    // namespace tools
}    // namespace vk_utils
