#include "vk_utils.hpp"

#include <fstream>
#include <stdexcept>

#include "vk_init.hpp"

void vk_utils::vk_try(vk::Result res)
{
    if (res < vk::Result::eSuccess) { throw VulkanException(res); }
}

void vk_utils::vk_try(VkResult res) { vk_try(vk::Result(res)); }

std::vector<std::byte> vk_utils::readFile(const std::string &filename)
{
    size_t fileSize = 0;
    std::vector<std::byte> fileContent;
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) { throw std::runtime_error("failed to open file " + filename); }
    fileSize = file.tellg();
    fileContent.resize(fileSize);
    file.seekg(0);
    file.read((char *)fileContent.data(), fileSize);
    file.close();
    return fileContent;
}

vk::ShaderModule vk_utils::createShaderModule(const vk::Device &device, const std::vector<std::byte> &code)
{
    auto createInfo = vk_init::populateVkShaderModuleCreateInfo(code);
    return device.createShaderModule(createInfo);
}

vk::Format vk_utils::findSupportedFormat(vk::PhysicalDevice &gpu, const std::vector<vk::Format> &candidates,
                                         vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (vk::Format format: candidates) {
        vk::FormatProperties props = gpu.getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format");
}

uint32_t vk_utils::findMemoryType(vk::PhysicalDevice &physicalDevice, uint32_t typeFilter,
                                  vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties)) { return i; }
    }
    throw VulkanException("failed to find suitable memory type !");
}

const std::string vk_utils::tools::to_string(vk::SampleCountFlagBits count)
{
    switch (count) {
        case vk::SampleCountFlagBits::e1: return "No MSAA";
        case vk::SampleCountFlagBits::e2: return "2X MSAA";
        case vk::SampleCountFlagBits::e4: return "4X MSAA";
        case vk::SampleCountFlagBits::e8: return "8X MSAA";
        case vk::SampleCountFlagBits::e16: return "16X MSAA";
        case vk::SampleCountFlagBits::e32: return "32X MSAA";
        case vk::SampleCountFlagBits::e64: return "64X MSAA";
        default: return "Unknown";
    }
}
const std::string vk_utils::tools::to_string(vk::CullModeFlagBits count)
{
    switch (count) {
        case vk::CullModeFlagBits::eNone: return "No culling";
        case vk::CullModeFlagBits::eBack: return "Back culling";
        case vk::CullModeFlagBits::eFront: return "Front culling";
        case vk::CullModeFlagBits::eFrontAndBack: return "Both side culling";
        default: return "Unknown";
    }
}
