#pragma once

#include <Window.hpp>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "DeletionQueue.hpp"
#include "types/CreationParameters.hpp"

class Window;

class Swapchain
{
public:
    Swapchain();
    ~Swapchain();

    void init(Window &win, VkPhysicalDevice &gpu, VkDevice &device, VkSurfaceKHR &surface);
    void destroy();
    void recreate(Window &win, VkPhysicalDevice &gpu, VkDevice &device, VkSurfaceKHR &surface);
    uint32_t nbOfImage() const;

    constexpr const VkSwapchainKHR &getSwapchain() const { return swapChain; }
    constexpr float getAspectRatio() const
    {
        return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
    }
    constexpr VkImage &getSwapchainImage(const unsigned index) { return swapChainImages.at(index); }
    constexpr VkImageView &getSwapchainImageView(const unsigned index) { return swapChainImageViews.at(index); }
    constexpr const VkFormat &getSwapchainFormat() const { return swapChainImageFormat; }
    constexpr const VkExtent2D &getSwapchainExtent() const { return swapChainExtent; }
    constexpr const VkExtent3D getSwapchainExtent3D(uint32_t depth = 1) const
    {
        return {
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .depth = depth,
        };
    }

private:
    void createSwapchain(Window &win, VkPhysicalDevice &gpu, VkDevice &device, VkSurfaceKHR &surface);
    void getImages(VkDevice &device);
    void createImageViews(VkDevice &device);

private:
    DeletionQueue chainDeletionQueue;
    VkExtent2D swapChainExtent = {.width = 0, .height = 0};
    VkFormat swapChainImageFormat;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
};
