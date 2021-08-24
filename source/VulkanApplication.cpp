#include "VulkanApplication.hpp"

#include <Window.hpp>
#include <array>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>

#include "Camera.hpp"
#include "DebugMacros.hpp"
#include "Logger.hpp"
#include "PipelineBuilder.hpp"
#include "QueueFamilyIndices.hpp"
#include "imgui.h"
#include "types/Material.hpp"
#include "types/Vertex.hpp"
#include "types/VulkanException.hpp"
#include "types/vk_types.hpp"
#include "vk_init.hpp"
#include "vk_mem_alloc.h"
#include "vk_utils.hpp"

#define MAX_OBJECT 1000
#define MAX_COMMANDS 100
#define MAX_MATERIALS 100

PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                              const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugUtilsMessengerEXT *pMessenger)
{
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
                                                           VkAllocationCallbacks const *pAllocator)
{
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

VulkanApplication::VulkanApplication(): window("Vulkan", 800, 600)
{
    DEBUG_FUNCTION
    window.setUserPointer(this);
    window.setResizeCallback(framebufferResizeCallback);
}

VulkanApplication::~VulkanApplication()
{
    DEBUG_FUNCTION
    if (device) device.waitIdle();
    swapchain.destroy();
    swapchainDeletionQueue.flush();
    mainDeletionQueue.flush();
}

void VulkanApplication::init(std::function<bool()> &&loadingStage)
{
    DEBUG_FUNCTION
    initInstance();
    initDebug();
    initSurface();
    pickPhysicalDevice();
    createLogicalDevice();

    swapchain.init(window, physical_device, device, surface);

    createAllocator();
    createSyncObjects();
    createIndirectBuffer();
    createRenderPass();
    createDescriptorSetLayout();
    createTextureDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();

    createImgui();

    if (!loadingStage()) { throw std::runtime_error("Loading stage failed !"); }

    createTextureSampler();
    createTextureDescriptorSets();
    createCommandBuffers();
}

void VulkanApplication::initInstance()
{
    DEBUG_FUNCTION
    if (enableValidationLayers && !checkValiationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    auto debugInfo = vk_init::populateDebugUtilsMessengerCreateInfoEXT(&VulkanApplication::debugCallback);
    auto extensions = getRequiredExtensions(enableValidationLayers);

    vk::ApplicationInfo applicationInfo{
        .apiVersion = VK_API_VERSION_1_2,
    };
    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    if (enableValidationLayers) {
        createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT *)&debugInfo;
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    instance = vk::createInstance(createInfo);
    mainDeletionQueue.push([&] { instance.destroy(); });
}

void VulkanApplication::initDebug()
{
    DEBUG_FUNCTION
    if (!enableValidationLayers) return;
    auto debugInfo = vk_init::populateDebugUtilsMessengerCreateInfoEXT(&VulkanApplication::debugCallback);

    pfnVkCreateDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    pfnVkDestroyDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

    instance.createDebugUtilsMessengerEXT(debugInfo);
    logger->warn("Validation Layers") << "Validation Layers are activated !";
    LOGGER_ENDL;
    mainDeletionQueue.push([&] { instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger); });
}

void VulkanApplication::pickPhysicalDevice()
{
    DEBUG_FUNCTION
    for (const auto &device: vk_utils::getPhysicalDevices(instance)) {
        if (isDeviceSuitable(device, surface)) {
            physical_device = device;
            maxMsaaSample = getMexUsableSampleCount(physical_device);
            creationParameters.msaaSample = maxMsaaSample;
            break;
        }
    }
    if (!physical_device) {
        throw VulkanException("failed to find suitable GPU");
    } else {
        vk::PhysicalDeviceProperties deviceProperties = physical_device.getProperties();
        logger->info(vk_utils::tools::physicalDeviceTypeString(deviceProperties.deviceType))
            << deviceProperties.deviceName;
        LOGGER_ENDL;
    }
}

void VulkanApplication::initSurface()
{
    DEBUG_FUNCTION
    window.createSurface(instance, &surface);
    mainDeletionQueue.push([&] { instance.destroy(surface); });
}

void VulkanApplication::createLogicalDevice()
{
    DEBUG_FUNCTION
    float fQueuePriority = 1.0f;
    auto indices = QueueFamilyIndices::findQueueFamilies(physical_device, surface);
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies{indices.graphicsFamily.value(), indices.presentFamily.value()};

    for (const uint32_t queueFamily: uniqueQueueFamilies) {
        queueCreateInfos.push_back(vk_init::populateDeviceQueueCreateInfo(1, queueFamily, fQueuePriority));
    }

    vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndex{
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
    };
    vk::PhysicalDeviceVulkan11Features v11Features{
        .pNext = &descriptorIndex,
        .shaderDrawParameters = VK_TRUE,
    };

    vk::PhysicalDeviceFeatures deviceFeature{
        .fillModeNonSolid = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };
    vk::DeviceCreateInfo createInfo{
        .pNext = &v11Features,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeature,
    };
    device = physical_device.createDevice(createInfo);
    mainDeletionQueue.push([=] { device.destroy(); });

    graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
    presentQueue = device.getQueue(indices.presentFamily.value(), 0);
}

void VulkanApplication::createAllocator()
{
    DEBUG_FUNCTION
    vma::AllocatorCreateInfo allocatorInfo;
    allocatorInfo.physicalDevice = physical_device;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorInfo.frameInUseCount = MAX_FRAME_FRAME_IN_FLIGHT;

    allocator = vma::createAllocator(allocatorInfo);
    mainDeletionQueue.push([&] { allocator.destroy(); });
}

void VulkanApplication::createRenderPass()
{
    DEBUG_FUNCTION
    vk::AttachmentDescription colorAttachment{
        .format = swapchain.getSwapchainFormat(),
        .samples = creationParameters.msaaSample,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eColorAttachmentOptimal,
    };
    vk::AttachmentDescription depthAttachment{
        .format = vk_utils::findSupportedFormat(
            physical_device, {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment),
        .samples = creationParameters.msaaSample,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };
    vk::AttachmentDescription colorAttachmentResolve{
        .format = swapchain.getSwapchainFormat(),
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
    };
    vk::AttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };
    vk::AttachmentReference depthAttachmentRef{
        .attachment = 1,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };
    vk::AttachmentReference colorAttachmentResolveRef{
        .attachment = 2,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };
    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = &colorAttachmentResolveRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };
    vk::SubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask =
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstStageMask =
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eNoneKHR,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    };

    std::array<vk::AttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    vk::RenderPassCreateInfo renderPassInfo{
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };
    renderPass = device.createRenderPass(renderPassInfo);

    swapchainDeletionQueue.push([&] { device.destroy(renderPass); });
}
void VulkanApplication::createGraphicsPipeline()
{
    DEBUG_FUNCTION
    auto vertShaderCode = vk_utils::readFile("shaders/default_triangle.vert.spv");
    auto fragShaderCode = vk_utils::readFile("shaders/default_triangle.frag.spv");

    auto vertShaderModule = vk_utils::createShaderModule(device, vertShaderCode);
    auto fragShaderModule = vk_utils::createShaderModule(device, fragShaderCode);

    std::vector<vk::PushConstantRange> pipelinePushConstant = {
        vk::PushConstantRange{
            .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            .offset = 0,
            .size = sizeof(Camera::GPUCameraData),
        },
    };

    std::vector<vk::DescriptorSetLayout> setLayout = {descriptorSetLayout, texturesSetLayout};
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .setLayoutCount = static_cast<uint32_t>(setLayout.size()),
        .pSetLayouts = setLayout.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pipelinePushConstant.size()),
        .pPushConstantRanges = pipelinePushConstant.data(),
    };
    pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

    std::vector<vk::VertexInputBindingDescription> binding = {Vertex::getBindingDescription()};
    std::vector<vk::VertexInputAttributeDescription> attribute = Vertex::getAttributeDescriptons();

    PipelineBuilder builder;
    builder.pipelineLayout = pipelineLayout;
    builder.shaderStages.push_back(
        vk_init::populateVkPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex, vertShaderModule));
    builder.shaderStages.push_back(
        vk_init::populateVkPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment, fragShaderModule));
    builder.vertexInputInfo = vk_init::populateVkPipelineVertexInputStateCreateInfo(binding, attribute);
    builder.inputAssembly =
        vk_init::populateVkPipelineInputAssemblyCreateInfo(vk::PrimitiveTopology::eTriangleList, VK_FALSE);
    builder.multisampling = vk_init::populateVkPipelineMultisampleStateCreateInfo(creationParameters.msaaSample);
    builder.depthStencil = vk_init::populateVkPipelineDepthStencilStateCreateInfo();
    builder.viewport.x = 0.0f;
    builder.viewport.y = 0.0f;
    builder.viewport.width = swapchain.getSwapchainExtent().width;
    builder.viewport.height = swapchain.getSwapchainExtent().height;
    builder.viewport.minDepth = 0.0f;
    builder.viewport.maxDepth = 1.0f;
    builder.scissor.offset = vk::Offset2D{0, 0};
    builder.scissor.extent = swapchain.getSwapchainExtent();
    builder.colorBlendAttachment = vk_init::populateVkPipelineColorBlendAttachmentState();
    builder.rasterizer = vk_init::populateVkPipelineRasterizationStateCreateInfo(creationParameters.polygonMode);
    builder.rasterizer.cullMode = creationParameters.cullMode;
    builder.rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    graphicsPipeline = builder.build(device, renderPass);

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    swapchainDeletionQueue.push([&] {
        device.destroy(graphicsPipeline);
        device.destroy(pipelineLayout);
    });
}

void VulkanApplication::createFramebuffers()
{
    DEBUG_FUNCTION
    swapChainFramebuffers.resize(swapchain.nbOfImage());
    for (size_t i = 0; i < swapchain.nbOfImage(); i++) {
        std::array<vk::ImageView, 3> attachments = {colorImage.imageView, depthResources.imageView,
                                                    swapchain.getSwapchainImageView(i)};

        vk::FramebufferCreateInfo framebufferInfo{
            .renderPass = renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = swapchain.getSwapchainExtent().width,
            .height = swapchain.getSwapchainExtent().height,
            .layers = 1,
        };

        device.createFramebuffer(framebufferInfo);
    }
    swapchainDeletionQueue.push([&] {
        for (auto &framebuffer: swapChainFramebuffers) { device.destroy(framebuffer); }
    });
}

void VulkanApplication::createCommandPool()
{
    DEBUG_FUNCTION
    auto indices = QueueFamilyIndices::findQueueFamilies(physical_device, surface);
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = indices.graphicsFamily.value(),
    };
    commandPool = device.createCommandPool(poolInfo);
    uploadContext.commandPool = device.createCommandPool(poolInfo);
    mainDeletionQueue.push([&] {
        device.destroy(uploadContext.commandPool);
        device.destroy(commandPool);
    });
}

void VulkanApplication::createCommandBuffers()
{
    DEBUG_FUNCTION
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = static_cast<uint32_t>(swapchain.nbOfImage()),
    };
    commandBuffers = device.allocateCommandBuffers(allocInfo);
    swapchainDeletionQueue.push([&] { device.free(commandPool, commandBuffers); });
}

void VulkanApplication::createSyncObjects()
{
    DEBUG_FUNCTION
    vk::SemaphoreCreateInfo semaphoreInfo{};
    vk::FenceCreateInfo fenceInfo{
        .flags = vk::FenceCreateFlagBits::eSignaled,
    };
    vk::FenceCreateInfo uploadFenceInfo{};

    uploadContext.uploadFence = device.createFence(uploadFenceInfo);
    for (auto &f: frames) {
        f.imageAvailableSemaphore = device.createSemaphore(semaphoreInfo);
        f.renderFinishedSemaphore = device.createSemaphore(semaphoreInfo);
        f.inFlightFences = device.createFence(fenceInfo);
    }

    mainDeletionQueue.push([&] {
        device.destroy(uploadContext.uploadFence);
        for (auto &f: frames) {
            device.destroy(f.inFlightFences);
            device.destroy(f.renderFinishedSemaphore);
            device.destroy(f.imageAvailableSemaphore);
        }
    });
}

void VulkanApplication::createDescriptorSetLayout()
{
    DEBUG_FUNCTION
    vk::DescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .pImmutableSamplers = nullptr,
    };
    vk::DescriptorSetLayoutBinding materialBinding{
        .binding = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {uboLayoutBinding, materialBinding};
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
    mainDeletionQueue.push([&] { device.destroy(descriptorSetLayout); });
}

void VulkanApplication::createTextureDescriptorSetLayout()
{
    DEBUG_FUNCTION
    std::vector<vk::DescriptorBindingFlags> flags{
        vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::ePartiallyBound,
    };

    vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingInfo{
        .bindingCount = static_cast<uint32_t>(flags.size()),
        .pBindingFlags = flags.data(),
    };
    vk::DescriptorSetLayoutBinding samplerLayoutBiding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 32,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };
    vk::DescriptorSetLayoutCreateInfo texturesSetLayoutInfo{
        .pNext = &bindingInfo,
        .bindingCount = 1,
        .pBindings = &samplerLayoutBiding,
    };
    texturesSetLayout = device.createDescriptorSetLayout(texturesSetLayoutInfo);
    mainDeletionQueue.push([&] { device.destroy(texturesSetLayout); });
}

void VulkanApplication::createUniformBuffers()
{
    DEBUG_FUNCTION
    for (auto &f: frames) {
        f.data.uniformBuffers = createBuffer(sizeof(gpuObject::UniformBufferObject) * MAX_OBJECT,
                                             vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eCpuToGpu);
        f.data.materialBuffer = createBuffer(sizeof(gpuObject::Material) * MAX_MATERIALS,
                                             vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eCpuToGpu);
    }
    swapchainDeletionQueue.push([&] {
        for (auto &f: frames) {
            allocator.destroyBuffer(f.data.uniformBuffers.buffer, f.data.uniformBuffers.memory);
            allocator.destroyBuffer(f.data.materialBuffer.buffer, f.data.materialBuffer.memory);
        }
    });
}

void VulkanApplication::createIndirectBuffer()
{
    DEBUG_FUNCTION
    for (auto &f: frames) {
        f.indirectBuffer =
            this->createBuffer(sizeof(vk::DrawIndexedIndirectCommand) * MAX_OBJECT,
                               vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer |
                                   vk::BufferUsageFlagBits::eIndirectBuffer,
                               vma::MemoryUsage::eCpuToGpu);
    }
    mainDeletionQueue.push([&] {
        for (auto &f: frames) { allocator.destroyBuffer(f.indirectBuffer.buffer, f.indirectBuffer.memory); }
    });
}
void VulkanApplication::createDescriptorPool()
{
    DEBUG_FUNCTION
    vk::DescriptorPoolSize poolSize[] = {
        {
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = MAX_FRAME_FRAME_IN_FLIGHT,
        },
        {
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 100,
        },
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        .maxSets = static_cast<uint32_t>(std::accumulate(poolSize, poolSize + std::size(poolSize), 0,
                                                         [](auto prev, auto &i) { return prev + i.descriptorCount; })),
        .poolSizeCount = std::size(poolSize),
        .pPoolSizes = poolSize,
    };
    descriptorPool = device.createDescriptorPool(poolInfo);
    swapchainDeletionQueue.push([&] { device.destroy(descriptorPool); });
}

void VulkanApplication::createDescriptorSets()
{
    DEBUG_FUNCTION
    for (auto &f: frames) {
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &descriptorSetLayout,
        };

        // store index 0 because we only use one
        f.data.objectDescriptor = device.allocateDescriptorSets(allocInfo)[0];

        vk::DescriptorBufferInfo bufferInfo{
            .buffer = f.data.uniformBuffers.buffer,
            .offset = 0,
            .range = sizeof(gpuObject::UniformBufferObject) * MAX_OBJECT,
        };
        vk::DescriptorBufferInfo materialInfo{
            .buffer = f.data.materialBuffer.buffer,
            .offset = 0,
            .range = sizeof(gpuObject::Material) * MAX_MATERIALS,
        };
        std::vector<vk::WriteDescriptorSet> descriptorWrites{
            {
                .dstSet = f.data.objectDescriptor,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &bufferInfo,
            },
            {
                .dstSet = f.data.objectDescriptor,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &materialInfo,
            },
        };
        device.updateDescriptorSets(descriptorWrites, 0);
    }
}

void VulkanApplication::createTextureDescriptorSets()
{
    DEBUG_FUNCTION
    std::vector<vk::DescriptorImageInfo> imagesInfos;
    for (auto &[_, t]: loadedTextures) {
        imagesInfos.push_back(vk::DescriptorImageInfo{
            .sampler = textureSampler,
            .imageView = t.imageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        });
    }

    uint32_t counts[] = {static_cast<uint32_t>(imagesInfos.size())};
    vk::DescriptorSetVariableDescriptorCountAllocateInfo set_counts{
        .descriptorSetCount = std::size(counts),
        .pDescriptorCounts = counts,
    };
    vk::DescriptorSetAllocateInfo allocInfo{
        .pNext = &set_counts,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &texturesSetLayout,
    };
    texturesSet = device.allocateDescriptorSets(allocInfo)[0];

    vk::WriteDescriptorSet descriptorWrite{
        .dstSet = texturesSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = static_cast<uint32_t>(imagesInfos.size()),
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = imagesInfos.data(),
    };
    device.updateDescriptorSets(descriptorWrite, 0);
}

void VulkanApplication::createTextureSampler()
{
    DEBUG_FUNCTION
    vk::PhysicalDeviceProperties properties = physical_device.getProperties();

    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = static_cast<float>(mipLevels),
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = VK_FALSE,
    };
    device.createSampler(samplerInfo);
    mainDeletionQueue.push([&] { device.destroy(textureSampler); });
}

void VulkanApplication::createDepthResources()
{
    DEBUG_FUNCTION
    vk::Format depthFormat = vk_utils::findSupportedFormat(
        physical_device, {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);

    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = depthFormat,
        .extent = swapchain.getSwapchainExtent3D(),
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = creationParameters.msaaSample,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };
    vma::AllocationCreateInfo allocInfo{};
    allocInfo.usage = vma::MemoryUsage::eGpuOnly;
    std::tie(depthResources.image, depthResources.memory) = allocator.createImage(imageInfo, allocInfo);

    auto createInfo = vk_init::populateVkImageViewCreateInfo(depthResources.image, depthFormat);
    createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depthResources.imageView = device.createImageView(createInfo);

    transitionImageLayout(depthResources.image, depthFormat, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eDepthStencilAttachmentOptimal);
    swapchainDeletionQueue.push([&] {
        device.destroy(depthResources.imageView);
        allocator.destroyImage(depthResources.image, depthResources.memory);
    });
}

void VulkanApplication::createColorResources()
{
    DEBUG_FUNCTION
    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = swapchain.getSwapchainFormat(),
        .extent = swapchain.getSwapchainExtent3D(),
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = creationParameters.msaaSample,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };
    vma::AllocationCreateInfo allocInfo{};
    allocInfo.usage = vma::MemoryUsage::eGpuOnly;
    std::tie(colorImage.image, colorImage.memory) = allocator.createImage(imageInfo, allocInfo);

    auto createInfo = vk_init::populateVkImageViewCreateInfo(colorImage.image, swapchain.getSwapchainFormat());
    createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    colorImage.imageView = device.createImageView(createInfo);

    swapchainDeletionQueue.push([&] {
        device.destroy(colorImage.imageView);
        allocator.destroyImage(colorImage.image, colorImage.memory);
    });
}

void VulkanApplication::createImgui()
{
    DEBUG_FUNCTION
    vk::DescriptorPoolSize pool_sizes[] = {{vk::DescriptorType::eSampler, 1000},
                                           {vk::DescriptorType::eCombinedImageSampler, 1000},
                                           {vk::DescriptorType::eSampledImage, 1000},
                                           {vk::DescriptorType::eStorageImage, 1000},
                                           {vk::DescriptorType::eUniformTexelBuffer, 1000},
                                           {vk::DescriptorType::eStorageTexelBuffer, 1000},
                                           {vk::DescriptorType::eUniformBuffer, 1000},
                                           {vk::DescriptorType::eStorageBuffer, 1000},
                                           {vk::DescriptorType::eUniformBufferDynamic, 1000},
                                           {vk::DescriptorType::eStorageBufferDynamic, 1000},
                                           {vk::DescriptorType::eInputAttachment, 1000}};

    vk::DescriptorPoolCreateInfo pool_info = {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1000,
        .poolSizeCount = std::size(pool_sizes),
        .pPoolSizes = pool_sizes,
    };

    vk::DescriptorPool imguiPool;
    imguiPool = device.createDescriptorPool(pool_info);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window.getWindow(), true);

    ImGui_ImplVulkan_InitInfo init_info{
        .Instance = instance,
        .PhysicalDevice = physical_device,
        .Device = device,
        .Queue = graphicsQueue,
        .DescriptorPool = imguiPool,
        .MinImageCount = swapchain.nbOfImage(),
        .ImageCount = swapchain.nbOfImage(),
        .MSAASamples = static_cast<VkSampleCountFlagBits>(creationParameters.msaaSample),
    };
    ImGui_ImplVulkan_Init(&init_info, renderPass);

    immediateCommand([&](vk::CommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    swapchainDeletionQueue.push([=, this] {
        vkDestroyDescriptorPool(device, imguiPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    });
}

void VulkanApplication::recreateSwapchain()
{
    DEBUG_FUNCTION
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

    swapchain.recreate(window, physical_device, device, surface);
    createRenderPass();
    createGraphicsPipeline();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createTextureDescriptorSets();
    createCommandBuffers();
    createImgui();
    logger->info("Swapchain") << "Swapchain recreation complete... { height=" << swapchain.getSwapchainExtent().height
                              << ", width =" << swapchain.getSwapchainExtent().width
                              << ", number = " << swapchain.nbOfImage() << " }";
    LOGGER_ENDL;
    ImGui_ImplVulkan_SetMinImageCount(swapchain.nbOfImage());
}
