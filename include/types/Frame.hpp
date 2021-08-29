#pragma once

#include "types/AllocatedBuffer.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct Frame {
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    vk::Fence inFlightFences;
    AllocatedBuffer indirectBuffer{};
    struct {
        AllocatedBuffer uniformBuffers{};
        AllocatedBuffer materialBuffer{};
        vk::DescriptorSet objectDescriptor = VK_NULL_HANDLE;
    } data = {};
};
