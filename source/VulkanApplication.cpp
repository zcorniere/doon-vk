#include "VulkanApplication.hpp"
#include "Logger.hpp"
#include "PipelineBuilder.hpp"
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
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool(queueIndices);
    createCommandBuffers();
    createSyncObjects();
}

void VulkanApplication::initInstance()
{
    if (enableValidationLayers && !checkValiationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    auto debugInfo = vk_init::populateDebugUtilsMessengerCreateInfoEXT(&VulkanApplication::debugCallback);
    auto extensions = getRequiredExtensions(enableValidationLayers);

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

void VulkanApplication::createRenderPass()
{
    VkAttachmentDescription colorAttachment{
        .format = swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };
    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };
    VK_TRY(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
    mainDeletionQueue.push([&]() { vkDestroyRenderPass(device, renderPass, nullptr); });
}
void VulkanApplication::createGraphicsPipeline()
{
    auto vertShaderCode = vk_utils::readFile("shaders/default_triangle.vert.spv");
    auto fragShaderCode = vk_utils::readFile("shaders/default_triangle.frag.spv");

    auto vertShaderModule = vk_utils::createShaderModule(device, vertShaderCode);
    auto fragShaderModule = vk_utils::createShaderModule(device, fragShaderCode);

    std::vector<VkVertexInputBindingDescription> binding;
    std::vector<VkVertexInputAttributeDescription> attribute;

    auto pipelineLayoutCreateInfo = vk_init::empty::populateVkPipelineLayoutCreateInfo();
    VK_TRY(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

    PipelineBuilder builder;
    builder.pipelineLayout = pipelineLayout;
    builder.shaderStages.push_back(
        vk_init::populateVkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
    builder.shaderStages.push_back(
        vk_init::populateVkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));
    builder.vertexInputInfo = vk_init::populateVkPipelineVertexInputStateCreateInfo(binding, attribute);
    builder.inputAssembly =
        vk_init::populateVkPipelineInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
    builder.multisampling = vk_init::populateVkPipelineMultisampleStateCreateInfo();
    builder.viewport.x = 0.0f;
    builder.viewport.y = 0.0f;
    builder.viewport.width = swapChainExtent.width;
    builder.viewport.height = swapChainExtent.height;
    builder.viewport.minDepth = 0.0f;
    builder.viewport.maxDepth = 1.0f;
    builder.scissor.offset = {0, 0};
    builder.scissor.extent = swapChainExtent;
    builder.colorBlendAttachment = vk_init::populateVkPipelineColorBlendAttachmentState();
    builder.rasterizer = vk_init::populateVkPipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
    graphicsPipeline = builder.build(device, renderPass);

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    mainDeletionQueue.push([&]() {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    });
}

void VulkanApplication::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        VK_TRY(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
    }
    mainDeletionQueue.push([&]() {
        for (auto &framebuffer: swapChainFramebuffers) { vkDestroyFramebuffer(device, framebuffer, nullptr); }
    });
}

void VulkanApplication::createCommandPool(const QueueFamilyIndices &indices)
{
    VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = indices.graphicsFamily.value(),
    };
    VK_TRY(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
    mainDeletionQueue.push([&]() { vkDestroyCommandPool(device, commandPool, nullptr); });
}

void VulkanApplication::createCommandBuffers()
{
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
    };
    VK_TRY(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()));
    for (size_t i = 0; i < commandBuffers.size(); ++i) {
        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr,
        };

        VkClearValue clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
        VkRenderPassBeginInfo renderPassInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = swapChainFramebuffers[i],
            .renderArea =
                {
                    .offset = {0, 0},
                    .extent = swapChainExtent,
                },
            .clearValueCount = 1,
            .pClearValues = &clearColor,
        };

        VK_TRY(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffers[i]);
        VK_TRY(vkEndCommandBuffer(commandBuffers[i]));
    }
}

void VulkanApplication::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
    };
    VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (auto &f: frames) {
        VK_TRY(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &f.imageAvailableSemaphore));
        VK_TRY(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &f.renderFinishedSemaphore));
        VK_TRY(vkCreateFence(device, &fenceInfo, nullptr, &f.inFlightFences));
    }

    mainDeletionQueue.push([&]() {
        for (const auto f: frames) {
            vkDestroyFence(device, f.inFlightFences, nullptr);
            vkDestroySemaphore(device, f.renderFinishedSemaphore, nullptr);
            vkDestroySemaphore(device, f.imageAvailableSemaphore, nullptr);
        }
    });
}