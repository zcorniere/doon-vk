#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <vulkan/vulkan.h>

namespace gpuObject
{
struct Material {
    glm::vec3 ambientColor;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

}    // namespace gpuObject
