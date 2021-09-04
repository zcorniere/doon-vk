#include "PipelineBuilder.hpp"

#include <iostream>

vk::Pipeline PipelineBuilder::build(vk::Device device, vk::RenderPass pass, vk::PipelineCache pipelineCache)
{
    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = VK_FALSE,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .layout = pipelineLayout,
        .renderPass = pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
    };

    vk::Pipeline newPipeline{};
    vk::Result result;
    std::tie(result, newPipeline) = device.createGraphicsPipeline(pipelineCache, pipelineInfo);
    if (result != vk::Result::eSuccess) {
        std::cout << "failed to create pipeline" << std::endl;
        return VK_NULL_HANDLE;
    } else {
        return newPipeline;
    }
}
