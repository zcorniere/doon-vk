#pragma once

#include "MemoryAllocator.hpp"
#include "types/Vertex.hpp"
#include <vector>

struct CPUMesh {
    std::vector<Vertex> verticies;
    std::vector<uint16_t> indices;
};

struct GPUMesh {
    AllocatedBuffer vertices;
    AllocatedBuffer indices;
};