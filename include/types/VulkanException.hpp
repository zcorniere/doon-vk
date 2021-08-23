#pragma once

#include <stdexcept>
#include <vulkan/vulkan.hpp>

struct VulkanException : public std::runtime_error {
    VulkanException(const std::string message): std::runtime_error(message) {}
    VulkanException(const vk::Result er): std::runtime_error(errorString(er)) {}

    static std::string errorString(vk::Result errorCode);
};
