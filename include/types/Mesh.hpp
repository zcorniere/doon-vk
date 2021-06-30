#pragma once

#include "MemoryAllocator.hpp"
#include "types/Vertex.hpp"
#include <vector>

struct CPUMesh {
    std::vector<Vertex> verticies;
    std::vector<uint32_t> indices;
};

struct GPUMesh {
    CPUMesh baseMesh;
    AllocatedBuffer vertices;
    AllocatedBuffer indices;
};