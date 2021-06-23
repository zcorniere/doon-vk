#include "VulkanApplication.hpp"

void VulkanApplication::copyBuffer(const VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize &size)
{
    immediateCommand([=](VkCommandBuffer &cmd) {
        VkBufferCopy copyRegion{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size,
        };
        vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);
    });
}
void VulkanApplication::copyBufferToImage(const VkBuffer &srcBuffer, VkImage &dstImage, uint32_t width, uint32_t height)
{
    immediateCommand([=](VkCommandBuffer &cmd) {
        VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1},
        };
        vkCmdCopyBufferToImage(cmd, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    });
}

void VulkanApplication::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                                    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                                    VkDeviceMemory &imageMemory)
{
    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent =
            {
                .width = width,
                .height = height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VK_TRY(vkCreateImage(device, &imageInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(physical_device, memRequirements.memoryTypeBits, properties),
    };
    VK_TRY(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));
    vkBindImageMemory(device, image, imageMemory, 0);
}

void VulkanApplication::immediateCommand(std::function<void(VkCommandBuffer &)> &&function)
{
    VkCommandBufferAllocateInfo cmdAllocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = uploadContext.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd{};
    VK_TRY(vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd));

    VkCommandBufferBeginInfo cmdBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    VK_TRY(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_TRY(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };

    VK_TRY(vkQueueSubmit(graphicsQueue, 1, &submit, uploadContext.uploadFence));

    VK_TRY(vkWaitForFences(device, 1, &uploadContext.uploadFence, true, 999999999999999));
    VK_TRY(vkResetFences(device, 1, &uploadContext.uploadFence));
    VK_TRY(vkResetCommandPool(device, uploadContext.commandPool, 0));
}

void VulkanApplication::transitionImageLayout(VkImage &image, VkFormat, VkImageLayout oldLayout,
                                              VkImageLayout newLayout)
{
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },

    };

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    immediateCommand([&](VkCommandBuffer &cmd) {
        vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    });
}