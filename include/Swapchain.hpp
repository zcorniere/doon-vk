#pragma once

#include <stdint.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "DeletionQueue.hpp"
#include "Window.hpp"
#include "types/CreationParameters.hpp"

class Swapchain
{
public:
    Swapchain();
    ~Swapchain();

    void init(Window &win, vk::PhysicalDevice &gpu, vk::Device &device, vk::SurfaceKHR &surface);
    void destroy();
    void recreate(Window &win, vk::PhysicalDevice &gpu, vk::Device &device, vk::SurfaceKHR &surface);
    uint32_t nbOfImage() const;

    constexpr const vk::SwapchainKHR &getSwapchain() const noexcept { return swapChain; }
    constexpr float getAspectRatio() const noexcept
    {
        return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
    }
    inline vk::Image &getSwapchainImage(const unsigned index) { return swapChainImages.at(index); }
    inline vk::ImageView &getSwapchainImageView(const unsigned index) { return swapChainImageViews.at(index); }
    constexpr const vk::Format &getSwapchainFormat() const noexcept { return swapChainImageFormat; }
    constexpr const vk::Extent2D &getSwapchainExtent() const noexcept { return swapChainExtent; }
    constexpr const vk::Extent3D getSwapchainExtent3D(uint32_t depth = 1) const noexcept
    {
        return {
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .depth = depth,
        };
    }

    inline operator bool() const { return swapChain; }

private:
    void createSwapchain(Window &win, vk::PhysicalDevice &gpu, vk::Device &device, vk::SurfaceKHR &surface);
    void getImages(vk::Device &device);
    void createImageViews(vk::Device &device);

private:
    DeletionQueue chainDeletionQueue;
    vk::Extent2D swapChainExtent;
    vk::Format swapChainImageFormat;

    vk::SwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
};
