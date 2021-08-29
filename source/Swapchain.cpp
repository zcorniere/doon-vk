#include "Swapchain.hpp"

#include <optional>
#include <stddef.h>
#include <stdexcept>

#include "DebugMacros.hpp"
#include "QueueFamilyIndices.hpp"
#include "SwapChainSupportDetails.hpp"
#include "vk_init.hpp"
#include "vk_utils.hpp"

class Window;

Swapchain::Swapchain() {}

Swapchain::~Swapchain() {}

void Swapchain::init(Window &win, vk::PhysicalDevice &gpu, vk::Device &device, vk::SurfaceKHR &surface)
{
    DEBUG_FUNCTION
    createSwapchain(win, gpu, device, surface);
    getImages(device);
    createImageViews(device);
}

void Swapchain::destroy() { chainDeletionQueue.flush(); }

void Swapchain::recreate(Window &win, vk::PhysicalDevice &gpu, vk::Device &device, vk::SurfaceKHR &surface)
{
    DEBUG_FUNCTION
    this->destroy();
    this->init(win, gpu, device, surface);
}

uint32_t Swapchain::nbOfImage() const
{
    if (swapChainImages.size() != swapChainImageViews.size()) [[unlikely]]
        throw std::length_error("swapchain has different ammout of vk::Image and vk::ImageView");
    return swapChainImages.size();
}

void Swapchain::createSwapchain(Window &window, vk::PhysicalDevice &gpu, vk::Device &device, vk::SurfaceKHR &surface)
{
    DEBUG_FUNCTION
    auto indices = QueueFamilyIndices::findQueueFamilies(gpu, surface);

    auto swapChainSupport = SwapChainSupportDetails::querySwapChainSupport(gpu, surface);
    auto surfaceFormat = swapChainSupport.chooseSwapSurfaceFormat();
    auto presentMode = swapChainSupport.chooseSwapPresentMode();
    auto extent = swapChainSupport.chooseSwapExtent(window);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = nullptr,
    };
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;        // Optional
        createInfo.pQueueFamilyIndices = nullptr;    // Optional
    }

    swapChain = device.createSwapchainKHR(createInfo);
    chainDeletionQueue.push([&]() { device.destroy(swapChain); });
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Swapchain::getImages(vk::Device &device)
{
    DEBUG_FUNCTION swapChainImages = device.getSwapchainImagesKHR(swapChain);
}

void Swapchain::createImageViews(vk::Device &device)
{
    DEBUG_FUNCTION
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        vk::ImageViewCreateInfo createInfo{
            .image = swapChainImages[i],
            .format = swapChainImageFormat,
        };
        swapChainImageViews.at(i) = device.createImageView(createInfo);
    }
    chainDeletionQueue.push([&]() {
        for (auto &imageView: swapChainImageViews) { vkDestroyImageView(device, imageView, nullptr); }
    });
}
