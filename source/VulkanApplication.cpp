#include "VulkanApplication.hpp"
#include "Logger.hpp"
#include "QueueFamilyIndices.hpp"
#include "vk_init.hpp"

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                              const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugUtilsMessengerEXT *pMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    return func(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
                                                           VkAllocationCallbacks const *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) { func(instance, messenger, pAllocator); }
}

VulkanApplication::VulkanApplication()
{
    if (enableValidationLayers && !checkValiationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    auto extensions = getRequiredExtensions(enableValidationLayers);

    vk::ApplicationInfo applicationInfo = {
        .pApplicationName = "doon",
        .applicationVersion = 1,
        .pEngineName = "No engine",
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_2,
    };

    vk::InstanceCreateInfo createInfo = {
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    auto debugInfo = vk_init::populateDebugUtilsMessengerCreateInfoEXT(&VulkanApplication::debugCallback);

    if (enableValidationLayers) {
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugInfo;
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    instance = vk::createInstance(createInfo);
    debugUtilsMessenger = instance.createDebugUtilsMessengerEXT(debugInfo);
}
VulkanApplication::~VulkanApplication()
{
    device.destroy();
    instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);
    instance.destroy();
}
void VulkanApplication::init(Window &win)
{
    pickPhysicalDevice();
    logger->info("GPU_PICKED") << physical_device.getProperties().deviceName;
    LOGGER_ENDL;
    createLogicalDevice();
}

void VulkanApplication::pickPhysicalDevice()
{
    auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty()) throw std::runtime_error("failed to find GPUs with Vulkan support");
    for (const auto &device: devices) {
        if (isDeviceSuitable(device)) {
            physical_device = device;
            break;
        }
    }
}

void VulkanApplication::createLogicalDevice()
{
    float fQueuePriority = 0.0f;
    auto indices = QueueFamilyIndices::findQueueFamilies(physical_device);
    vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = indices.graphicsFamily.value(),
        .queueCount = 1,
        .pQueuePriorities = &fQueuePriority,
    };
    vk::PhysicalDeviceFeatures deviceFeature{};

    vk::DeviceCreateInfo createInfo{
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .pEnabledFeatures = &deviceFeature,
    };

    device = physical_device.createDevice(createInfo);
    graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
}
