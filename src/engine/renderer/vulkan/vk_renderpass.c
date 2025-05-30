#include "engine/renderer/vulkan/vk_renderpass.h"

#include "engine/core/logger.h"
#include "engine/memory/memory.h"
#include "engine/renderer/vulkan/vk_type.h"

void vk_renderpass_init(vulkan_context_t *ctx, vec4 render_area,
                        vec4 clear_color, float depth, uint32_t stencil,
                        uint8_t clear_flags, b8 has_prev_pass, b8 has_next_pass,
                        vulkan_renderpass_t *renderpass) {
	renderpass->render_area = render_area;
	renderpass->clear_color = clear_color;
	renderpass->depth = depth;
	renderpass->stencil = stencil;
	renderpass->clear_flags = clear_flags;
	renderpass->has_prev_pass = has_prev_pass;
	renderpass->has_next_pass = has_next_pass;

	VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;

	uint32_t attach_desc_count = 0;
	VkAttachmentDescription attach_desc[2];

	/* Color Attachment */
	b8 do_clear_color = (renderpass->clear_flags & CLEAR_COLOR_BUFFER) != 0;
	VkAttachmentDescription color_attach = {};
    color_attach.format         = ctx->swapchain.image_format.format;
    color_attach.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attach.loadOp         = do_clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR
												 : VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attach.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    color_attach.initialLayout  =
        has_prev_pass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                      : VK_IMAGE_LAYOUT_UNDEFINED;

    color_attach.finalLayout    = 
		has_next_pass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
					  : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attach_desc[attach_desc_count] = color_attach;
	attach_desc_count++;

    VkAttachmentReference color_attach_ref = {};
    color_attach_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attach_ref.attachment = 0;

	subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &color_attach_ref;

    /* Depth Attachment */
	// b8 has_depth = depth;
    b8 do_clear_depth = (renderpass->clear_flags & CLEAR_DEPTH_BUFFER) != 0;
	if (do_clear_depth) {
		VkAttachmentDescription depth_attach = {};
		depth_attach.format         = ctx->device.depth_format;
		depth_attach.samples        = VK_SAMPLE_COUNT_1_BIT;
        depth_attach.loadOp  		= do_clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                    : VK_ATTACHMENT_LOAD_OP_LOAD;
        depth_attach.storeOp 		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attach.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attach.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		attach_desc[attach_desc_count] = depth_attach;
		attach_desc_count++;

		VkAttachmentReference depth_attach_ref = {};
		depth_attach_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth_attach_ref.attachment = 1;

        subpass.pDepthStencilAttachment = &depth_attach_ref;
	} else {
		memory_zero(&attach_desc[attach_desc_count], sizeof(VkAttachmentDescription));
        subpass.pDepthStencilAttachment = 0;
	}

	/* Dependency */
	VkSubpassDependency depend = {};
    depend.srcSubpass      = VK_SUBPASS_EXTERNAL;
    depend.dstSubpass      = 0;
    depend.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    depend.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    depend.srcAccessMask   = 0;
    depend.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	depend.dependencyFlags = 0;

    /* Create Renderpass */
	VkRenderPassCreateInfo renderpass_info = {};
    renderpass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_info.attachmentCount = attach_desc_count;
    renderpass_info.pAttachments    = attach_desc;
    renderpass_info.subpassCount    = 1;
    renderpass_info.pSubpasses      = &subpass;
    renderpass_info.dependencyCount = 1;
    renderpass_info.pDependencies   = &depend;
	renderpass_info.pNext 			= 0;
	renderpass_info.flags 			= 0;

    VK_CHECK(vkCreateRenderPass(ctx->device.logic_dev, &renderpass_info,
                                ctx->alloc, &renderpass->handle));
	ar_DEBUG("Vulkan Renderpass Created");
}

void vk_renderpass_begin(vulkan_commandbuffer_t *combuff,
                         vulkan_renderpass_t    *renderpass,
                         VkFramebuffer           framebuffer) {
    VkRenderPassBeginInfo begin_info = {};
    begin_info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass            = renderpass->handle;
    begin_info.framebuffer           = framebuffer;
    begin_info.renderArea.offset.x   = renderpass->render_area.x;
    begin_info.renderArea.offset.y   = renderpass->render_area.y;
    begin_info.renderArea.extent.width  = renderpass->render_area.z;
    begin_info.renderArea.extent.height = renderpass->render_area.w;
    begin_info.clearValueCount          = 0;
    begin_info.pClearValues             = 0;

    VkClearValue clear_values[2];
	memory_zero(clear_values, sizeof(VkClearValue) * 2);

    b8 do_clear_color = (renderpass->clear_flags & CLEAR_COLOR_BUFFER) != 0;
    b8 do_clear_depth = (renderpass->clear_flags & CLEAR_DEPTH_BUFFER) != 0;
    if (do_clear_color) {
        memory_copy(clear_values[begin_info.clearValueCount].color.float32,
                    renderpass->clear_color.elements, sizeof(float) * 4);
        begin_info.clearValueCount++;
    }

    if (do_clear_depth) {
        memory_copy(clear_values[begin_info.clearValueCount].color.float32,
                    renderpass->clear_color.elements, sizeof(float) * 4);
        clear_values[begin_info.clearValueCount].depthStencil.depth =
            renderpass->depth;

        b8 do_clear_stencil =
            (renderpass->clear_flags & CLEAR_STENCIL_BUFFER) != 0;
        clear_values[begin_info.clearValueCount].depthStencil.stencil =
            do_clear_stencil ? renderpass->stencil : 0;
        begin_info.clearValueCount++;
    }

    begin_info.pClearValues = begin_info.clearValueCount > 0 ? clear_values : 0;

    vkCmdBeginRenderPass(combuff->handle, &begin_info,
                         VK_SUBPASS_CONTENTS_INLINE);
}

void vk_renderpass_end(vulkan_commandbuffer_t *combuff,
                       vulkan_renderpass_t    *renderpass) {
    (void)renderpass;
    vkCmdEndRenderPass(combuff->handle);
}

void vk_renderpass_shut(vulkan_context_t    *ctx,
                        vulkan_renderpass_t *renderpass) {
    if (renderpass && renderpass->handle) {
        vkDestroyRenderPass(ctx->device.logic_dev, renderpass->handle,
                            ctx->alloc);
        renderpass->handle = 0;
    }
}
