#pragma once
#include <vulkan/vulkan.hpp>

struct CreationParameters {
    vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
    vk::SampleCountFlagBits msaaSample = vk::SampleCountFlagBits::e1;
    vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eFrontAndBack;
};
