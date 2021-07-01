#include "MemoryAllocator.hpp"
#include "vk_init.hpp"
#include "vk_utils.hpp"

MemoryAllocator::MemoryAllocator(VkPhysicalDevice &physical_device, VkDevice &device)
    : physical_device(physical_device), device(device)
{
}

MemoryAllocator::~MemoryAllocator(){};

AllocatedBuffer MemoryAllocator::alloc(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    AllocatedBuffer buffer{};
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_TRY(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer.buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = vk_utils::findMemoryType(physical_device, memRequirements.memoryTypeBits, properties),
    };
    VK_TRY(vkAllocateMemory(device, &allocInfo, nullptr, &buffer.memory));
    vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0);
    return buffer;
}

AllocatedImage MemoryAllocator::alloc(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                      VkMemoryPropertyFlags properties, bool bCreateImageView)
{
    AllocatedImage image{};
    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VK_TRY(vkCreateImage(device, &imageInfo, nullptr, &image.image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = vk_utils::findMemoryType(physical_device, memRequirements.memoryTypeBits, properties),
    };
    VK_TRY(vkAllocateMemory(device, &allocInfo, nullptr, &image.memory));
    vkBindImageMemory(device, image.image, image.memory, 0);

    if (bCreateImageView) {
        auto createInfo = vk_init::populateVkImageViewCreateInfo(image.image, VK_FORMAT_R8G8B8A8_SRGB);
        VK_TRY(vkCreateImageView(device, &createInfo, nullptr, &image.imageView))
    }
    return image;
}

void MemoryAllocator::free(AllocatedBuffer &buffer)
{
    vkDestroyBuffer(device, buffer.buffer, nullptr);
    vkFreeMemory(device, buffer.memory, nullptr);
}

void MemoryAllocator::free(AllocatedImage &image)
{
    vkDestroyImageView(device, image.imageView, nullptr);
    vkDestroyImage(device, image.image, nullptr);
    vkFreeMemory(device, image.memory, nullptr);
}