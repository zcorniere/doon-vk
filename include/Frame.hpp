#pragma once

#include <vulkan/vulkan.h>

struct Frame {
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFences;
};