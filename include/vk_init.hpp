#pragma once

#include <cstddef>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace vk_init
{
vk::InstanceCreateInfo populateVkInstanceCreateInfo(vk::ApplicationInfo &appCreateInfo,
                                                    const std::vector<const char *> &vExtentions,
                                                    const std::vector<const char *> &vLayers) noexcept;

vk::DebugUtilsMessengerCreateInfoEXT
populateDebugUtilsMessengerCreateInfoEXT(PFN_vkDebugUtilsMessengerCallbackEXT callback) noexcept;
vk::DeviceQueueCreateInfo populateDeviceQueueCreateInfo(const uint32_t, const uint32_t, const float &) noexcept;
vk::ImageViewCreateInfo populateVkImageViewCreateInfo(vk::Image &image, vk::Format format,
                                                      uint32_t mipLevel = 1) noexcept;
vk::ShaderModuleCreateInfo populateVkShaderModuleCreateInfo(const std::vector<std::byte> &code) noexcept;

vk::PipelineShaderStageCreateInfo populateVkPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits stage,
                                                                          vk::ShaderModule &module,
                                                                          const char *entryPoint = "main") noexcept;
vk::PipelineVertexInputStateCreateInfo populateVkPipelineVertexInputStateCreateInfo(
    const std::vector<vk::VertexInputBindingDescription> &binding,
    const std::vector<vk::VertexInputAttributeDescription> &attribute) noexcept;
vk::PipelineInputAssemblyStateCreateInfo populateVkPipelineInputAssemblyCreateInfo(vk::PrimitiveTopology,
                                                                                   vk::Bool32 = VK_FALSE) noexcept;
vk::PipelineRasterizationStateCreateInfo populateVkPipelineRasterizationStateCreateInfo(vk::PolygonMode) noexcept;
vk::PipelineMultisampleStateCreateInfo populateVkPipelineMultisampleStateCreateInfo(
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1) noexcept;
vk::PipelineColorBlendAttachmentState populateVkPipelineColorBlendAttachmentState() noexcept;

vk::PipelineLayoutCreateInfo
populateVkPipelineLayoutCreateInfo(const std::vector<vk::DescriptorSetLayout> &setLayout,
                                   const std::vector<vk::PushConstantRange> &pushLayout) noexcept;

vk::PipelineDepthStencilStateCreateInfo populateVkPipelineDepthStencilStateCreateInfo() noexcept;
vk::PushConstantRange populateVkPushConstantRange(vk::ShaderStageFlags stage, uint32_t size,
                                                  uint32_t offset = 0) noexcept;
}    // namespace vk_init
