#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <vulkan/vulkan.h>

struct Material {
    std::optional<VkDescriptorSet> textureSet;
    glm::vec3 ambientColor;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};