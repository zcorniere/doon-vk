#include "VulkanApplication.hpp"
#include "Logger.hpp"
#include "SwapChainSupportDetails.hpp"
#include "vk_init.hpp"
#include <set>

VulkanApplication::VulkanApplication() {}
VulkanApplication::~VulkanApplication() { mainDeletionQueue.flush(); }
void VulkanApplication::init(Window &win)
{
    initInstance();
    initDebug();
    initSurface(win);
    pickPhysicalDevice();
    auto queueIndices = createLogicalDevice();
    createSwapchain(queueIndices, win);
    createImageWiews();
}

void VulkanApplication::initInstance()
{
    if (enableValidationLayers && !checkValiationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    auto debugInfo = vk_init::populateDebugUtilsMessengerCreateInfoEXT(&VulkanApplication::debugCallback);
    auto extensions = getRequiredExtensions(enableValidationLayers);

    for (const auto &i: extensions) { std::cout << i << " | "; }
    std::cout << std::endl;
    VkApplicationInfo applicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_1,
    };
    VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    if (enableValidationLayers) {
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugInfo;
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    VK_TRY(vkCreateInstance(&createInfo, nullptr, &instance));
    mainDeletionQueue.push([&]() { vkDestroyInstance(instance, nullptr); });
}

void VulkanApplication::initDebug()
{
    if (!enableValidationLayers) return;

    auto debugInfo = vk_init::populateDebugUtilsMessengerCreateInfoEXT(&VulkanApplication::debugCallback);
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        VK_TRY(func(instance, &debugInfo, nullptr, &debugUtilsMessenger));
    } else {
        throw VulkanException("VK_ERROR_EXTENSION_NOT_PRESENT");
    }
    mainDeletionQueue.push([&]() {
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) { func(instance, debugUtilsMessenger, nullptr); }
    });
}
void VulkanApplication::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    if (devices.empty()) throw VulkanException("failed to find GPUs with Vulkan support");
    for (const auto &device: devices) {
        if (isDeviceSuitable(device, surface)) {
            physical_device = device;
            break;
        }
    }
    if (physical_device == VK_NULL_HANDLE) {
        throw VulkanException("failed to find suitable GPU");
    } else {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physical_device, &deviceProperties);
        logger->info(vk_utils::tools::physicalDeviceTypeString(deviceProperties.deviceType))
            << deviceProperties.deviceName;
        LOGGER_ENDL;
    }
}

void VulkanApplication::initSurface(Window &win)
{
    win.createSurface(instance, &surface);
    mainDeletionQueue.push([&]() { vkDestroySurfaceKHR(instance, surface, nullptr); });
}

QueueFamilyIndices VulkanApplication::createLogicalDevice()
{
    float fQueuePriority = 1.0f;
    auto indices = QueueFamilyIndices::findQueueFamilies(physical_device, surface);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies{indices.graphicsFamily.value(), indices.presentFamily.value()};

    for (const uint32_t queueFamily: uniqueQueueFamilies) {
        queueCreateInfos.push_back(vk_init::populateDeviceQueueCreateInfo(1, queueFamily, fQueuePriority));
    }

    VkPhysicalDeviceFeatures deviceFeature{};

    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeature,
    };
    VK_TRY(vkCreateDevice(physical_device, &createInfo, nullptr, &device));
    mainDeletionQueue.push([=]() { vkDestroyDevice(device, nullptr); });

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    return indices;
}

void VulkanApplication::createSwapchain(const QueueFamilyIndices &indices, Window &win)
{
    auto swapChainSupport = SwapChainSupportDetails::querySwapChainSupport(physical_device, surface);
    auto surfaceFormat = swapChainSupport.chooseSwapSurfaceFormat();
    auto presentMode = swapChainSupport.chooseSwapPresentMode();
    auto extent = swapChainSupport.chooseSwapExtent(win);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{

        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
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
        .oldSwapchain = this->swapChain,
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
    mainDeletionQueue.push([&]() { vkDestroySwapchainKHR(device, swapChain, nullptr); });
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanApplication::createImageWiews()
{
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        auto createInfo = vk_init::populateVkImageViewCreateInfo(swapChainImages.at(i), swapChainImageFormat);
        VK_TRY(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews.at(i)));
    }
    mainDeletionQueue.push([&]() {
        for (auto &imageView: swapChainImageViews) { vkDestroyImageView(device, imageView, nullptr); }
    });
}