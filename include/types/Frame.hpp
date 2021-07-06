#pragma once

#include "types/AllocatedBuffer.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct FrameData {
    AllocatedBuffer uniformBuffers = {};
    AllocatedBuffer materialBuffer = {};
    VkDescriptorSet objectDescriptor = VK_NULL_HANDLE;
};

struct Frame {
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFences;

    FrameData data = {};
};
