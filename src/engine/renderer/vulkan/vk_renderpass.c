#include "engine/renderer/vulkan/vk_renderpass.h"

#include "engine/core/logger.h"

void vk_renderpass_init(vulkan_context_t    *ctx,
                        vulkan_renderpass_t *renderpass) {
	const uint32_t attach_desc_count = 2;
	VkAttachmentDescription attach_desc[2];

	/* Color Attachment */
    attach_desc[0].format         = ctx->swapchain.image_format.format;
    attach_desc[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attach_desc[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attach_desc[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attach_desc[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attach_desc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attach_desc[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attach_desc[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	/* Depth Attachment */
	attach_desc[1].format         = ctx->device.depth_format;
    attach_desc[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attach_desc[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attach_desc[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attach_desc[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attach_desc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attach_desc[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attach_desc[1].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attach_ref = {};
    color_attach_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attach_ref.attachment = 0;

    VkAttachmentReference depth_attach_ref = {};
    depth_attach_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attach_ref.attachment = 1;

	/* Subpass */
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &color_attach_ref;
    subpass.pDepthStencilAttachment = &depth_attach_ref;

	/* Dependency */
	VkSubpassDependency depend = {};
    depend.srcSubpass      = VK_SUBPASS_EXTERNAL;
    depend.dstSubpass      = 0;
    depend.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    depend.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    depend.srcAccessMask   = 0;
    depend.dstAccessMask   = 0;

	/* Create Renderpass */
	VkRenderPassCreateInfo renderpass_info = {};
    renderpass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_info.attachmentCount = attach_desc_count;
    renderpass_info.pAttachments    = attach_desc;
    renderpass_info.subpassCount    = 1;
    renderpass_info.pSubpasses      = &subpass;
    renderpass_info.dependencyCount = 1;
    renderpass_info.pDependencies   = &depend;

    VK_CHECK(vkCreateRenderPass(ctx->device.logic_dev, &renderpass_info,
                                ctx->alloc, &renderpass->handle));
	ar_DEBUG("Vulkan Renderpass Created");
}

void vk_renderpass_begin(vulkan_commandbuffer_t *combuff,
                         vulkan_renderpass_t    *renderpass,
                         VkFramebuffer           framebuffer) {
    VkClearValue clear_values[2];
    clear_values[0].color =
        (VkClearColorValue){.float32 = {1.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil =
        (VkClearDepthStencilValue){.depth = 1.0f, .stencil = 0};

    VkRenderPassBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin_info.renderPass = renderpass->handle;
	begin_info.framebuffer = framebuffer;
	begin_info.renderArea.extent = renderpass->extents;
	begin_info.clearValueCount = 2;
	begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(combuff->handle, &begin_info,
                         VK_SUBPASS_CONTENTS_INLINE);
}

void vk_renderpass_end(vulkan_commandbuffer_t *combuff) {
	vkCmdEndRenderPass(combuff->handle);
}

void vk_renderpass_shut(vulkan_context_t *ctx, vulkan_renderpass_t *renderpass) {
	if (renderpass && renderpass->handle) {
		vkDestroyRenderPass(ctx->device.logic_dev, renderpass->handle, ctx->alloc);
		renderpass->handle = 0;
	}
}
