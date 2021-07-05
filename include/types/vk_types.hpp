#pragma once

#include <glm/glm.hpp>

namespace gpuObject
{

struct Transform {
    glm::mat4 translation;
    glm::mat4 rotation;
    glm::mat4 scale;
};

struct UniformBufferObject {
    Transform transform;
};

}    // namespace gpuObject
