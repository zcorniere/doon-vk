#pragma once

#include "types/AllocatedBuffer.hpp"
#include "types/Vertex.hpp"
#include <vector>

struct CPUMesh {
    std::vector<Vertex> verticies;
    std::vector<uint32_t> indices;
};

struct GPUMesh {
    VkDeviceSize verticiesOffset;
    VkDeviceSize verticiesSize;
    VkDeviceSize indicesOffset;
    VkDeviceSize indicesSize;
    AllocatedBuffer meshBuffer;
};
