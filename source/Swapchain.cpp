#include "Swapchain.hpp"

#include <optional>
#include <stddef.h>
#include <stdexcept>

#include "QueueFamilyIndices.hpp"
#include "SwapChainSupportDetails.hpp"
#include "vk_init.hpp"
#include "vk_utils.hpp"

class Window;

Swapchain::Swapchain() {}

Swapchain::~Swapchain() {}

void Swapchain::init(Window &win, VkPhysicalDevice &gpu, VkDevice &device, VkSurfaceKHR &surface)
{
    createSwapchain(win, gpu, device, surface);
    getImages(device);
    createImageViews(device);
}

void Swapchain::destroy() { chainDeletionQueue.flush(); }

void Swapchain::recreate(Window &win, VkPhysicalDevice &gpu, VkDevice &device, VkSurfaceKHR &surface)
{
    this->destroy();
    this->init(win, gpu, device, surface);
}

uint32_t Swapchain::nbOfImage() const
{
    if (swapChainImages.size() != swapChainImageViews.size()) [[unlikely]]
        throw std::length_error("swapchain has different ammout of VkImage and VkImageView");
    return swapChainImages.size();
}

void Swapchain::createSwapchain(Window &window, VkPhysicalDevice &gpu, VkDevice &device, VkSurfaceKHR &surface)
{
    auto indices = QueueFamilyIndices::findQueueFamilies(gpu, surface);

    auto swapChainSupport = SwapChainSupportDetails::querySwapChainSupport(gpu, surface);
    auto surfaceFormat = swapChainSupport.chooseSwapSurfaceFormat();
    auto presentMode = swapChainSupport.chooseSwapPresentMode();
    auto extent = swapChainSupport.chooseSwapExtent(window);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = nullptr,
    };
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;        // Optional
        createInfo.pQueueFamilyIndices = nullptr;    // Optional
    }
    VK_TRY(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));
    chainDeletionQueue.push([&]() { vkDestroySwapchainKHR(device, swapChain, nullptr); });
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Swapchain::getImages(VkDevice &device)
{
    uint32_t imageCount = 0;

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
}

void Swapchain::createImageViews(VkDevice &device)
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        auto createInfo = vk_init::populateVkImageViewCreateInfo(swapChainImages[i], swapChainImageFormat);
        VK_TRY(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]));
    }
    chainDeletionQueue.push([&]() {
        for (auto &imageView: swapChainImageViews) { vkDestroyImageView(device, imageView, nullptr); }
    });
}
