#include "assert.h"
#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() { return graphicsFamily.has_value(); }
    static QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice &device)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamiliesCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, queueFamilies.data());

        auto propertiesIter = std::find_if(queueFamilies.begin(), queueFamilies.end(),
                                           [](const auto &qfp) { return qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT; });

        indices.graphicsFamily = std::distance(queueFamilies.begin(), propertiesIter);
        assert(indices.graphicsFamily.value() < queueFamilies.size());
        return indices;
    }
};