#include "VulkanApplication.hpp"
#include "Logger.hpp"
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
        .apiVersion = VK_API_VERSION_1_1,
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
    instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);
    instance.destroy();
}
void VulkanApplication::init(Window &win) {}
