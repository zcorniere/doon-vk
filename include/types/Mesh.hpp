#pragma once

#include "types/AllocatedBuffer.hpp"
#include "types/Vertex.hpp"
#include <vector>

struct CPUMesh {
    std::vector<Vertex> verticies;
    std::vector<uint32_t> indices;
};

struct GPUMesh {
    VkDeviceSize verticiesOffset = 0;
    VkDeviceSize verticiesSize = 0;
    VkDeviceSize indicesOffset = 0;
    VkDeviceSize indicesSize = 0;
};
