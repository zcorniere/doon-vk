#include <vulkan/vulkan.hpp>

struct Texture {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;

    auto getDeletor(VkDevice device)
    {
        return [=]() {
            vkDestroyImageView(device, this->imageView, nullptr);
            vkFreeMemory(device, this->imageMemory, nullptr);
            vkDestroyImage(device, this->image, nullptr);
        };
    }
};