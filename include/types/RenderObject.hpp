#include "types/vk_types.hpp"
#include <string>

struct RenderObject {
    std::string meshID;
    gpuObject::UniformBufferObject ubo;
};
