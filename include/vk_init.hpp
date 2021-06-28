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
VkImageViewCreateInfo populateVkImageViewCreateInfo(VkImage &image, VkFormat format);
VkShaderModuleCreateInfo populateVkShaderModuleCreateInfo(const std::vector<std::byte> &code);

VkPipelineShaderStageCreateInfo populateVkPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
                                                                        VkShaderModule &module,
                                                                        const char *entryPoint = "main");
VkPipelineVertexInputStateCreateInfo
populateVkPipelineVertexInputStateCreateInfo(const std::vector<VkVertexInputBindingDescription> &binding,
                                             const std::vector<VkVertexInputAttributeDescription> &attribute);
VkPipelineInputAssemblyStateCreateInfo populateVkPipelineInputAssemblyCreateInfo(VkPrimitiveTopology,
                                                                                 VkBool32 = VK_FALSE);
VkPipelineRasterizationStateCreateInfo populateVkPipelineRasterizationStateCreateInfo(VkPolygonMode);
VkPipelineMultisampleStateCreateInfo populateVkPipelineMultisampleStateCreateInfo();
VkPipelineColorBlendAttachmentState populateVkPipelineColorBlendAttachmentState();

VkPipelineLayoutCreateInfo populateVkPipelineLayoutCreateInfo(const std::vector<VkDescriptorSetLayout> &setLayout,
                                                              const std::vector<VkPushConstantRange> &pushLayout);

VkPipelineDepthStencilStateCreateInfo populateVkPipelineDepthStencilStateCreateInfo();
VkPushConstantRange populateVkPushConstantRange(VkShaderStageFlags stage, uint32_t size, uint32_t offset = 0);

namespace empty
{
    VkPipelineLayoutCreateInfo populateVkPipelineLayoutCreateInfo();
}
}    // namespace vk_init