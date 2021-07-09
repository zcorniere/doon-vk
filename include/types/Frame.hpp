#pragma once

#include "types/AllocatedBuffer.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Frame {
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFences;
    AllocatedBuffer indirectBuffer = {};
    struct {
        AllocatedBuffer uniformBuffers = {};
        AllocatedBuffer materialBuffer = {};
        VkDescriptorSet objectDescriptor = VK_NULL_HANDLE;
    } data = {};
};
