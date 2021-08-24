#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "types/VulkanException.hpp"

#define VK_TRY(x)                                                        \
    {                                                                    \
        vk::Result err = x;                                              \
        if (err != vk::Result::eSuccess) { throw VulkanException(err); } \
    }

namespace vk_utils
{
template <typename T>
concept is_copyable = requires
{
    std::is_standard_layout_v<T>;
    typename std::vector<T>;
};
void vk_try(vk::Result res);
std::vector<std::byte> readFile(const std::string &filename);
vk::ShaderModule createShaderModule(const vk::Device &device, const std::vector<std::byte> &code);
vk::Format findSupportedFormat(vk::PhysicalDevice &gpu, const std::vector<vk::Format> &candidates,
                               vk::ImageTiling tiling, vk::FormatFeatureFlags features);
bool hasStencilComponent(vk::Format format);
uint32_t findMemoryType(vk::PhysicalDevice &physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

template <class... FailedValue>
bool isSwapchainInvalid(const vk::Result result, const FailedValue... failedResult)
{
    if (((result == failedResult) || ...)) {
        return true;
    } else {
        VK_TRY(result);
        return false;
    }
}
std::vector<vk::PhysicalDevice> getPhysicalDevices(vk::Instance &instance);

namespace tools
{
    const std::string to_string(vk::SampleCountFlagBits count);
    const std::string to_string(vk::CullModeFlagBits count);

    std::string physicalDeviceTypeString(vk::PhysicalDeviceType type);
}    // namespace tools
}    // namespace vk_utils
