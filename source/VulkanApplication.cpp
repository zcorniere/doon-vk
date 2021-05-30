#include "VulkanApplication.hpp"
#include "Logger.hpp"
#include "QueueFamilyIndices.hpp"
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
    createLogicalDevice();
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

void VulkanApplication::createLogicalDevice()
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
        .pEnabledFeatures = &deviceFeature,
    };
    VK_TRY(vkCreateDevice(physical_device, &createInfo, nullptr, &device));
    mainDeletionQueue.push([=]() { vkDestroyDevice(device, nullptr); });

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void VulkanApplication::initSurface(Window &win)
{
    win.createSurface(instance, &surface);
    mainDeletionQueue.push([&]() { vkDestroySurfaceKHR(instance, surface, nullptr); });
}