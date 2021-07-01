#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Frame {
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFences;
};
