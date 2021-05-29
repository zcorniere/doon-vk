#include <cstdint>
#include <optional>
#include <vulkan/vulkan.hpp>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() { return graphicsFamily.has_value(); }
    static QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice &device)
    {
        QueueFamilyIndices indices;
        auto queueFamiliesProperties = device.getQueueFamilyProperties();

        auto propertiesIter = std::find_if(
            queueFamiliesProperties.begin(), queueFamiliesProperties.end(),
            [](const vk::QueueFamilyProperties &qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; });

        indices.graphicsFamily = std::distance(queueFamiliesProperties.begin(), propertiesIter);
        assert(indices.graphicsFamily.value() < queueFamiliesProperties.size());
        return indices;
    }
};