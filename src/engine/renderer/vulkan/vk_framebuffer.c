#include "engine/renderer/vulkan/vk_framebuffer.h"

#include "engine/memory/memory.h"

void vk_framebuffer_init(vulkan_context_t *ctx, vulkan_renderpass_t *renderpass,
                         VkExtent2D extents, VkImageView *attach,
                         uint32_t attach_count, vulkan_framebuffer_t *fbuffer) {
    fbuffer->attach =
        memory_alloc(sizeof(VkImageView) * attach_count, MEMTAG_RENDERER);

    for (uint32_t i = 0; i < attach_count; ++i) {
		fbuffer->attach[i] = attach[i];
	}
	fbuffer->renderpass = renderpass;
	fbuffer->attach_count = attach_count;

	VkFramebufferCreateInfo finfo = {};
	finfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	finfo.renderPass = renderpass->handle;
	finfo.attachmentCount = attach_count;
	finfo.pAttachments = fbuffer->attach;
	finfo.layers = 1;
	finfo.width = extents.width;
	finfo.height = extents.height;

    VK_CHECK(vkCreateFramebuffer(ctx->device.logic_dev, &finfo, ctx->alloc,
                                 &fbuffer->handle));
}

void vk_framebuffer_shut(vulkan_context_t *ctx, vulkan_framebuffer_t *fbuffer) {
    vkDestroyFramebuffer(ctx->device.logic_dev, fbuffer->handle, ctx->alloc);

    if (fbuffer->attach) {
        memory_free(fbuffer->attach,
                    sizeof(VkImageView) * fbuffer->attach_count,
                    MEMTAG_RENDERER);
        fbuffer->attach = 0;
    }

    fbuffer->handle       = 0;
    fbuffer->attach_count = 0;
    fbuffer->renderpass   = 0;
}
