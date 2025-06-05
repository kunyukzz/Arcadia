#include "engine/renderer/vulkan/vk_pipeline.h"

#include "engine/core/logger.h"
#include "engine/memory/memory.h"
#include "engine/renderer/vulkan/vk_result.h"

b8 vk_pipeline_init(vulkan_context_t *ctx, vulkan_renderpass_t *renderpass,
                    uint32_t attr_count, uint32_t stride,
                    uint32_t set_layout_count, uint32_t stg_count,
                    VkVertexInputAttributeDescription *attr,
                    VkDescriptorSetLayout             *set_layout,
                    VkPipelineShaderStageCreateInfo *stg, VkViewport viewport,
                    VkRect2D scissor, b8 is_wireframe, b8 depth_enable,
                    uint32_t push_const_range_count, range *push_const_range,
                    vulkan_pipeline_t *pipeline) {
    (void)viewport;
    (void)scissor;
    /* Viewport State */
	VkPipelineViewportStateCreateInfo vw_info = {};
	vw_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vw_info.viewportCount = 1;
	vw_info.pViewports = NULL;
	vw_info.scissorCount = 1;
	vw_info.pScissors = NULL;

	/* Rasterize */
	VkPipelineRasterizationStateCreateInfo raster_info = {};
    raster_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.depthClampEnable        = VK_FALSE;
    raster_info.rasterizerDiscardEnable = VK_FALSE;
    raster_info.polygonMode =
        is_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    raster_info.lineWidth = 1;
    raster_info.cullMode = VK_CULL_MODE_BACK_BIT;
	raster_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster_info.depthBiasEnable = VK_FALSE;
	raster_info.depthBiasConstantFactor = 0.0f;
	raster_info.depthBiasClamp = 0.0f;
	raster_info.depthBiasSlopeFactor = 0.0f;

	/* Multisampling */
	VkPipelineMultisampleStateCreateInfo mlt_info = {};
	mlt_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	mlt_info.sampleShadingEnable = VK_FALSE;
	mlt_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	mlt_info.minSampleShading = 1.0f;
	mlt_info.pSampleMask = 0;
	mlt_info.alphaToCoverageEnable = VK_FALSE;
	mlt_info.alphaToOneEnable = VK_FALSE;

	/* Depth & Stencil */
	VkPipelineDepthStencilStateCreateInfo depth_stc_info = {};
    depth_stc_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	if (depth_enable) {
		depth_stc_info.depthTestEnable       = VK_TRUE;
		depth_stc_info.depthWriteEnable      = VK_TRUE;
		depth_stc_info.depthCompareOp        = VK_COMPARE_OP_LESS;
		depth_stc_info.depthBoundsTestEnable = VK_FALSE;
		depth_stc_info.stencilTestEnable     = VK_FALSE;
	}

    /* Color Blend */
	VkPipelineColorBlendAttachmentState blend_attach;
	memory_zero(&blend_attach, sizeof(VkPipelineColorBlendAttachmentState));
    blend_attach.blendEnable         = VK_TRUE;
    blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attach.colorBlendOp        = VK_BLEND_OP_ADD;
    blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attach.alphaBlendOp        = VK_BLEND_OP_ADD;
    blend_attach.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend_info = {};
	blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_info.logicOpEnable = VK_FALSE;
	blend_info.logicOp = VK_LOGIC_OP_COPY;
	blend_info.attachmentCount = 1;
	blend_info.pAttachments = &blend_attach;

	/* Dynamic State */
	VkDynamicState dyn_state[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dyn_info = {};
    dyn_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn_info.dynamicStateCount = 3;
    dyn_info.pDynamicStates    = dyn_state;

    /* Vertex Input */
    VkVertexInputBindingDescription bind_desc;
    bind_desc.binding   = 0;
    bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bind_desc.stride    = stride;

    /* Attributes */
    VkPipelineVertexInputStateCreateInfo vert_info = {};
    vert_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_info.vertexBindingDescriptionCount         = 1;
    vert_info.pVertexBindingDescriptions            = &bind_desc;
    vert_info.vertexAttributeDescriptionCount       = attr_count;
    vert_info.pVertexAttributeDescriptions          = attr;

    /* Input Assembly */
    VkPipelineInputAssemblyStateCreateInfo asm_info = {};
    asm_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    asm_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    asm_info.primitiveRestartEnable = VK_FALSE;

    /* Pipeline Layout */
	VkPipelineLayoutCreateInfo pipe_lay_info = {};
	pipe_lay_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	/* Push Constant */
	if (push_const_range_count > 0) {
		if (push_const_range_count > 32) {
            ar_ERROR("vk_pipeline_init - cannot have more than 32 push "
                     "constant range. Passed count: %i",
                     push_const_range_count);
            return false;
        }

		VkPushConstantRange range[32];
		memory_zero(range, sizeof(VkPushConstantRange) * 32);
		for (uint32_t i = 0; i < push_const_range_count; ++i) {
            range[i].stageFlags =
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            range[i].offset = push_const_range[i].offset;
            range[i].size = push_const_range[i].size;
		}
		pipe_lay_info.pPushConstantRanges = range;
		pipe_lay_info.pushConstantRangeCount = push_const_range_count;
	} else {
		pipe_lay_info.pPushConstantRanges = 0;
		pipe_lay_info.pushConstantRangeCount = 0;
	}

    /* Descriptor Layout */
	pipe_lay_info.setLayoutCount = set_layout_count;
	pipe_lay_info.pSetLayouts = set_layout;

	/* Create Pipeline Layout */
    VK_CHECK(vkCreatePipelineLayout(ctx->device.logic_dev, &pipe_lay_info,
                                    ctx->alloc, &pipeline->pipe_layout));

	/* Create Pipeline */
    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = stg_count;
    pipeline_info.pStages    = stg;
    pipeline_info.pVertexInputState   = &vert_info;
    pipeline_info.pInputAssemblyState = &asm_info;

    pipeline_info.pViewportState      = &vw_info;
    pipeline_info.pRasterizationState = &raster_info;
    pipeline_info.pMultisampleState   = &mlt_info;
    pipeline_info.pDepthStencilState  = depth_enable ? &depth_stc_info : 0;
    pipeline_info.pColorBlendState    = &blend_info;
    pipeline_info.pDynamicState       = &dyn_info;
    pipeline_info.pTessellationState  = 0;

    pipeline_info.layout              = pipeline->pipe_layout;

    pipeline_info.renderPass          = renderpass->handle;
    pipeline_info.subpass             = 0;
    pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex   = -1;

    VkResult result =
        vkCreateGraphicsPipelines(ctx->device.logic_dev, VK_NULL_HANDLE, 1,
                                  &pipeline_info, ctx->alloc,
                                  &pipeline->handle);
    if (vk_result_is_success(result)) {
        ar_DEBUG("Graphics Pipeline Created");
		return true;
    }

    ar_ERROR("Graphics Pipeline failed with: %s",
             vk_result_string(result, true));
    return false;
}

void vk_pipeline_shut(vulkan_context_t *ctx, vulkan_pipeline_t *pipeline) {
	if (pipeline) {
		if (pipeline->handle) {
            vkDestroyPipeline(ctx->device.logic_dev, pipeline->handle,
                              ctx->alloc);
            pipeline->handle = 0;
        }

		if (pipeline->pipe_layout) {
            vkDestroyPipelineLayout(ctx->device.logic_dev,
                                    pipeline->pipe_layout, ctx->alloc);
            pipeline->pipe_layout = 0;
		}
	}
}

void vk_pipeline_bind(vulkan_commandbuffer_t *combuff,
                      VkPipelineBindPoint     bind_point,
                      vulkan_pipeline_t      *pipeline) {
	vkCmdBindPipeline(combuff->handle, bind_point, pipeline->handle);
}



