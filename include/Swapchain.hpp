#pragma once

#include <Window.hpp>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "DeletionQueue.hpp"
#include "types/CreationParameters.hpp"

class Window;

class Swapchain
{
public:
    Swapchain();
    ~Swapchain();

    void init(Window &win, vk::PhysicalDevice &gpu, vk::Device &device, vk::SurfaceKHR &surface);
    void destroy();
    void recreate(Window &win, vk::PhysicalDevice &gpu, vk::Device &device, vk::SurfaceKHR &surface);
    uint32_t nbOfImage() const;

    constexpr const vk::SwapchainKHR &getSwapchain() const { return swapChain; }
    constexpr float getAspectRatio() const
    {
        return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
    }
    inline vk::Image &getSwapchainImage(const unsigned index) { return swapChainImages.at(index); }
    inline vk::ImageView &getSwapchainImageView(const unsigned index) { return swapChainImageViews.at(index); }
    constexpr const vk::Format &getSwapchainFormat() const { return swapChainImageFormat; }
    constexpr const vk::Extent2D &getSwapchainExtent() const { return swapChainExtent; }
    constexpr const vk::Extent3D getSwapchainExtent3D(uint32_t depth = 1) const
    {
        return {
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .depth = depth,
        };
    }

private:
    void createSwapchain(Window &win, vk::PhysicalDevice &gpu, vk::Device &device, vk::SurfaceKHR &surface);
    void getImages(vk::Device &device);
    void createImageViews(vk::Device &device);

private:
    DeletionQueue chainDeletionQueue;
    vk::Extent2D swapChainExtent = {.width = 0, .height = 0};
    vk::Format swapChainImageFormat;

    vk::SwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
};
