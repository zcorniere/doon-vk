#include "VulkanApplication.hpp"
#include "Logger.hpp"
#include "PipelineBuilder.hpp"
#include "SwapChainSupportDetails.hpp"
#include "vk_init.hpp"

#include <set>
#include <stb_image.h>
#include <tiny_obj_loader.h>

VulkanApplication::VulkanApplication(): window("Vulkan", 800, 600)
{
    window.setUserPointer(this);
    window.setResizeCallback(framebufferResizeCallback);
}

VulkanApplication::~VulkanApplication()
{
    vkDeviceWaitIdle(device);
    swapchainDeletionQueue.flush();
    mainDeletionQueue.flush();
}

void VulkanApplication::init()
{
    initInstance();
    initDebug();
    initSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
    createSwapchain();
    createSyncObjects();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createDepthResources();
    createFramebuffers();

    loadTextures();
    loadModel();
    createTextureSampler();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
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
        .apiVersion = VK_API_VERSION_1_2,
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

void VulkanApplication::initSurface()
{
    window.createSurface(instance, &surface);
    mainDeletionQueue.push([&]() { vkDestroySurfaceKHR(instance, surface, nullptr); });
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

    VkPhysicalDeviceFeatures deviceFeature{
        .samplerAnisotropy = VK_TRUE,
    };

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
}

void VulkanApplication::createAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo{
        .physicalDevice = physical_device,
        .device = device,
        .instance = instance,
        .vulkanApiVersion = VK_API_VERSION_1_2,
    };
    VK_TRY(vmaCreateAllocator(&allocatorInfo, &allocator));
    mainDeletionQueue.push([&]() { vmaDestroyAllocator(allocator); });
}
void VulkanApplication::createSwapchain()
{
    auto indices = QueueFamilyIndices::findQueueFamilies(physical_device, surface);

    auto swapChainSupport = SwapChainSupportDetails::querySwapChainSupport(physical_device, surface);
    auto surfaceFormat = swapChainSupport.chooseSwapSurfaceFormat();
    auto presentMode = swapChainSupport.chooseSwapPresentMode();
    auto extent = swapChainSupport.chooseSwapExtent(window);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
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
        .oldSwapchain = nullptr,
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
    swapchainDeletionQueue.push([&]() { vkDestroySwapchainKHR(device, swapChain, nullptr); });

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanApplication::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        auto createInfo = vk_init::populateVkImageViewCreateInfo(swapChainImages[i], swapChainImageFormat);
        VK_TRY(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]));
    }
    swapchainDeletionQueue.push([&]() {
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
    VkAttachmentDescription depthAttachment{
        .format = vk_utils::findSupportedFormat(
            physical_device, {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference depthAttachmentRef{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };
    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };
    VK_TRY(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
    swapchainDeletionQueue.push([&]() { vkDestroyRenderPass(device, renderPass, nullptr); });
}
void VulkanApplication::createGraphicsPipeline()
{
    auto vertShaderCode = vk_utils::readFile("shaders/default_triangle.vert.spv");
    auto fragShaderCode = vk_utils::readFile("shaders/default_triangle.frag.spv");

    auto vertShaderModule = vk_utils::createShaderModule(device, vertShaderCode);
    auto fragShaderModule = vk_utils::createShaderModule(device, fragShaderCode);

    auto pipelinePushConstant = vk_init::populateVkPushConstantRange(
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera::GPUCameraData));
    auto pipelineLayoutCreateInfo = vk_init::empty::populateVkPipelineLayoutCreateInfo();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pipelinePushConstant;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    VK_TRY(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

    std::vector<VkVertexInputBindingDescription> binding = {Vertex::getBindingDescription()};
    std::vector<VkVertexInputAttributeDescription> attribute = Vertex::getAttributeDescriptons();

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
    builder.depthStencil = vk_init::populateVkPipelineDepthStencilStateCreateInfo();
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
    builder.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    builder.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    graphicsPipeline = builder.build(device, renderPass);

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    swapchainDeletionQueue.push([&]() {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    });
}

void VulkanApplication::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthResources.imageView};

        VkFramebufferCreateInfo framebufferInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .layers = 1,
        };

        VK_TRY(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
    }
    swapchainDeletionQueue.push([&]() {
        for (auto &framebuffer: swapChainFramebuffers) { vkDestroyFramebuffer(device, framebuffer, nullptr); }
    });
}

void VulkanApplication::createCommandPool()
{
    auto indices = QueueFamilyIndices::findQueueFamilies(physical_device, surface);
    VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = indices.graphicsFamily.value(),
    };
    VK_TRY(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
    VK_TRY(vkCreateCommandPool(device, &poolInfo, nullptr, &uploadContext.commandPool));
    mainDeletionQueue.push([&]() {
        vkDestroyCommandPool(device, uploadContext.commandPool, nullptr);
        vkDestroyCommandPool(device, commandPool, nullptr);
    });
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
    swapchainDeletionQueue.push(
        [&]() { vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data()); });
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
    VkFenceCreateInfo uploadFenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    VK_TRY(vkCreateFence(device, &uploadFenceInfo, nullptr, &uploadContext.uploadFence));
    for (auto &f: frames) {
        VK_TRY(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &f.imageAvailableSemaphore));
        VK_TRY(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &f.renderFinishedSemaphore));
        VK_TRY(vkCreateFence(device, &fenceInfo, nullptr, &f.inFlightFences));
    }

    mainDeletionQueue.push([&]() {
        vkDestroyFence(device, uploadContext.uploadFence, nullptr);
        for (const auto f: frames) {
            vkDestroyFence(device, f.inFlightFences, nullptr);
            vkDestroySemaphore(device, f.renderFinishedSemaphore, nullptr);
            vkDestroySemaphore(device, f.imageAvailableSemaphore, nullptr);
        }
    });
}

void VulkanApplication::loadModel()
{
    for (auto &file: std::filesystem::directory_iterator("../models")) {
        if (file.path().extension() != ".obj") continue;
        logger->info("LOADING") << "Loading object: " << file.path();
        LOGGER_ENDL;

        CPUMesh mesh{};
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file.path().c_str(), nullptr)) {
            if (!warn.empty()) {
                logger->warn("LOADING_OBJ") << warn;
                LOGGER_ENDL;
            }
            if (!err.empty()) {
                logger->err("LOADING_OBJ") << err;
                LOGGER_ENDL;
                throw std::runtime_error("Error while loading obj file");
            }
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto &shape: shapes) {
            for (const auto &index: shape.mesh.indices) {
                Vertex vertex{
                    .pos =
                        {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2],
                        },
                    .color = {1.0f, 1.0f, 1.0f},
                    .texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                                 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]},
                };

                if (!uniqueVertices.contains(vertex)) {
                    uniqueVertices[vertex] = mesh.verticies.size();
                    mesh.verticies.push_back(vertex);
                }

                mesh.indices.push_back(uniqueVertices.at(vertex));
            }
        }
        loadedMeshes[file.path().stem()] = uploadMesh(mesh);
    }
    mainDeletionQueue.push([=]() {
        for (auto &[_, i]: loadedMeshes) {
            vmaDestroyBuffer(allocator, i.uniformBuffers.buffer, i.uniformBuffers.memory);
            vmaDestroyBuffer(allocator, i.meshBuffer.buffer, i.meshBuffer.memory);
        }
    });
}

void VulkanApplication::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding samplerLayoutBiding{
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };
    VkDescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBiding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };
    VK_TRY(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));
    mainDeletionQueue.push([&]() { vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr); });
}

void VulkanApplication::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    VmaAllocationCreateInfo allocInfo{
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    };
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    uniformBuffers.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VK_TRY(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &uniformBuffers[i].buffer, &uniformBuffers[i].memory,
                               nullptr));
    }
    swapchainDeletionQueue.push([&]() {
        for (auto &i: uniformBuffers) { vmaDestroyBuffer(allocator, i.buffer, i.memory); }
    });
}

void VulkanApplication::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    poolSizes.at(0).type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes.at(0).descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    poolSizes.at(1).type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes.at(1).descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .maxSets = static_cast<uint32_t>(swapChainImages.size()),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };
    VK_TRY(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
    swapchainDeletionQueue.push([&]() { vkDestroyDescriptorPool(device, descriptorPool, nullptr); });
}

void VulkanApplication::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(swapChainImages.size()),
        .pSetLayouts = layouts.data(),
    };
    descriptorSets.resize(swapChainImages.size());
    VK_TRY(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()));
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        VkDescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers[i].buffer,
            .offset = 0,
            .range = sizeof(UniformBufferObject),
        };
        VkDescriptorImageInfo imageInfo{
            .sampler = textureSampler,
            .imageView = loadedTextures.at("viking_room").imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites.at(0).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites.at(0).dstSet = descriptorSets[i];
        descriptorWrites.at(0).dstBinding = 0;
        descriptorWrites.at(0).dstArrayElement = 0;
        descriptorWrites.at(0).descriptorCount = 1;
        descriptorWrites.at(0).descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites.at(0).pBufferInfo = &bufferInfo;

        descriptorWrites.at(1).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites.at(1).dstSet = descriptorSets[i];
        descriptorWrites.at(1).dstBinding = 1;
        descriptorWrites.at(1).dstArrayElement = 0;
        descriptorWrites.at(1).descriptorCount = 1;
        descriptorWrites.at(1).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites.at(1).pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0,
                               nullptr);
    }
}

void VulkanApplication::loadTextures()
{
    for (auto &f: std::filesystem::directory_iterator("../textures")) {
        logger->info("LOADING") << "Loading texture: " << f.path();
        LOGGER_ENDL;
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(f.path().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) throw std::runtime_error("failed to load texture image");

        VkBufferCreateInfo stagingBufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .size = imageSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VmaAllocationCreateInfo stagingAllocInfo{
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        };
        AllocatedBuffer stagingBuffer{};
        VK_TRY(vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer.buffer,
                               &stagingBuffer.memory, nullptr));
        copyBuffer(stagingBuffer, pixels, imageSize);
        stbi_image_free(pixels);

        AllocatedImage image{};
        VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .extent =
                {
                    .width = static_cast<uint32_t>(texWidth),
                    .height = static_cast<uint32_t>(texHeight),
                    .depth = 1,
                },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        VmaAllocationCreateInfo allocInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        };
        VK_TRY(vmaCreateImage(allocator, &imageInfo, &allocInfo, &image.image, &image.memory, nullptr));
        auto createInfo = vk_init::populateVkImageViewCreateInfo(image.image, VK_FORMAT_R8G8B8A8_SRGB);
        VK_TRY(vkCreateImageView(device, &createInfo, nullptr, &image.imageView));

        transitionImageLayout(image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer.buffer, image.image, static_cast<uint32_t>(texWidth),
                          static_cast<uint32_t>(texHeight));
        transitionImageLayout(image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        loadedTextures.insert({f.path().stem(), std::move(image)});
        vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.memory);
    }
    mainDeletionQueue.push([&]() {
        for (auto &[_, p]: loadedTextures) {
            vkDestroyImageView(device, p.imageView, nullptr);
            vmaDestroyImage(allocator, p.image, p.memory);
        }
    });
}

void VulkanApplication::createTextureSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    VkSamplerCreateInfo samplerInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    VK_TRY(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler));
    mainDeletionQueue.push([&]() { vkDestroySampler(device, textureSampler, nullptr); });
}

void VulkanApplication::createDepthResources()
{
    VkFormat depthFormat = vk_utils::findSupportedFormat(
        physical_device, {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent =
            {
                .width = swapChainExtent.width,
                .height = swapChainExtent.height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo allocInfo{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };
    VK_TRY(vmaCreateImage(allocator, &imageInfo, &allocInfo, &depthResources.image, &depthResources.memory, nullptr));

    auto createInfo = vk_init::populateVkImageViewCreateInfo(depthResources.image, depthFormat);
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    VK_TRY(vkCreateImageView(device, &createInfo, nullptr, &depthResources.imageView));

    transitionImageLayout(depthResources.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    swapchainDeletionQueue.push([&]() {
        vkDestroyImageView(device, depthResources.imageView, nullptr);
        vmaDestroyImage(allocator, depthResources.image, depthResources.memory);
    });
}

void VulkanApplication::recreateSwapchain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window.getWindow(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window.getWindow(), &width, &height);
        glfwWaitEvents();
    }

    logger->info("Swapchain") << "Recreaing swapchain...";
    LOGGER_ENDL;
    vkDeviceWaitIdle(device);
    swapchainDeletionQueue.flush();

    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    logger->info("Swapchain") << "Swapchain recreation complete... { height=" << swapChainExtent.height
                              << ", width =" << swapChainExtent.width << "}";
    LOGGER_ENDL;
}

GPUMesh VulkanApplication::uploadMesh(const CPUMesh &mesh)
{
    VkDeviceSize verticesSize = sizeof(mesh.verticies[0]) * mesh.verticies.size();
    VkDeviceSize indicesSize = sizeof(mesh.indices[0]) * mesh.indices.size();
    VkDeviceSize uniformBuffersSize = sizeof(UniformBufferObject);
    VkDeviceSize totalSize = verticesSize + indicesSize;

    VkBufferCreateInfo stagingBufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = totalSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VmaAllocationCreateInfo stagingAllocInfo{
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    };
    VkBufferCreateInfo uniformBufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = uniformBuffersSize,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VmaAllocationCreateInfo uniformAllocInfo{
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    };
    VkBufferCreateInfo meshBufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = totalSize,
        .usage =
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VmaAllocationCreateInfo meshAllocInfo{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    GPUMesh gmesh{
        .verticiesOffset = 0,
        .verticiesSize = mesh.verticies.size(),
        .indicesOffset = verticesSize,
        .indicesSize = mesh.indices.size(),
    };
    AllocatedBuffer stagingBuffer{};
    VK_TRY(vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer.buffer,
                           &stagingBuffer.memory, nullptr));
    VK_TRY(vmaCreateBuffer(allocator, &uniformBufferInfo, &uniformAllocInfo, &gmesh.uniformBuffers.buffer,
                           &gmesh.uniformBuffers.memory, nullptr));
    VK_TRY(vmaCreateBuffer(allocator, &meshBufferInfo, &meshAllocInfo, &gmesh.meshBuffer.buffer,
                           &gmesh.meshBuffer.memory, nullptr));

    void *mapped = nullptr;
    vmaMapMemory(allocator, stagingBuffer.memory, &mapped);
    std::memcpy(mapped, mesh.verticies.data(), verticesSize);
    std::memcpy((unsigned char *)mapped + verticesSize, mesh.indices.data(), indicesSize);
    vmaUnmapMemory(allocator, stagingBuffer.memory);

    copyBuffer(stagingBuffer.buffer, gmesh.meshBuffer.buffer, totalSize);
    vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.memory);
    return gmesh;
}
