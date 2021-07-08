#include "types/vk_types.hpp"
#include <string>

struct RenderObject {
    std::string meshID;
    std::string objectName = meshID;
    gpuObject::UniformBufferObject ubo;
};
