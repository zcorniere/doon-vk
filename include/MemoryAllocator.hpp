#pragma once

#include <cstring>
#include <vector>
#include <vulkan/vulkan.h>

struct AllocatedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct AllocatedImage {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

class MemoryAllocator
{
public:
    MemoryAllocator(VkPhysicalDevice &physical_device, VkDevice &device);
    ~MemoryAllocator();
    AllocatedBuffer alloc(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    AllocatedImage alloc(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties, bool bCreateImageView = true);

    template <typename T>
    void copy(AllocatedBuffer &buffer, std::vector<T> data)
    {
        VkDeviceSize size = sizeof(data[0]) * data.size();
        void *mapped = nullptr;
        vkMapMemory(device, buffer.memory, 0, size, 0, &mapped);
        std::memcpy(mapped, data.data(), size);
        vkUnmapMemory(device, buffer.memory);
    }

    template <typename T>
    void copy(AllocatedBuffer &buffer, T *data, size_t size)
    {
        void *mapped = nullptr;
        vkMapMemory(device, buffer.memory, 0, size, 0, &mapped);
        std::memcpy(mapped, data, size);
        vkUnmapMemory(device, buffer.memory);
    }

    void free(AllocatedBuffer &buffer);
    void free(AllocatedImage &image);

private:
    VkPhysicalDevice &physical_device;
    VkDevice &device;
};