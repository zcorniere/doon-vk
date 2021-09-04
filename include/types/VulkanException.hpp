#pragma once

#include "DebugMacros.hpp"
#include <stdexcept>
#include <vulkan/vulkan.hpp>

struct VulkanException : public std::runtime_error {
    VulkanException(const std::string &message): std::runtime_error(message){DEBUG_FUNCTION};
    VulkanException(const vk::Result &er): std::runtime_error(vk::to_string(er)){DEBUG_FUNCTION};
    VulkanException(const VkResult &er): VulkanException(vk::Result(er)){};
    VulkanException(const vk::Error &e): VulkanException(e.what()){};
};

struct OutOfDateSwapchainError : public VulkanException {
    OutOfDateSwapchainError(const std::string &message = "OutOfDateSwapchainError"): VulkanException(message) {}
};
