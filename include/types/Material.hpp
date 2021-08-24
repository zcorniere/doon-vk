#pragma once

#include <glm/glm.hpp>

namespace gpuObject
{
struct Material {
    glm::vec4 ambientColor;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

}    // namespace gpuObject
