#pragma once

#include "MemoryAllocator.hpp"
#include "types/Vertex.hpp"
#include <vector>

struct UniformBufferObject {
    glm::mat4 translation;
    glm::mat4 rotation;
    glm::mat4 scale;
};
struct CPUMesh {
    std::vector<Vertex> verticies;
    std::vector<uint32_t> indices;
};

struct GPUMesh {
    VkDeviceSize verticiesOffset;
    VkDeviceSize verticiesSize;
    VkDeviceSize indicesOffset;
    VkDeviceSize indicesSize;
    AllocatedBuffer uniformBuffers;
    AllocatedBuffer meshBuffer;
};
