#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Frame {
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFences;
};

struct UniformBufferObject {
    glm::mat4 translation;
    glm::mat4 rotation;
    glm::mat4 scale;
};