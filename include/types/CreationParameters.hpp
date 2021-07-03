#pragma once
#include <vulkan/vulkan.h>

struct CreationParameters {
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkSampleCountFlagBits msaaSample = VK_SAMPLE_COUNT_1_BIT;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
};
