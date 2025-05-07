#include "engine/renderer/vulkan/vk_image.h"

#include "engine/core/logger.h"
#include "engine/memory/memory.h"

void vk_image_transition_layout(vulkan_context_t       *ctx,
                                vulkan_commandbuffer_t *combuff,
                                vulkan_image_t *image, VkFormat *format,
                                VkImageLayout old_layout,
                                VkImageLayout new_layout) {
	(void)format;
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = (uint32_t)ctx->device.graphic_idx;
	barrier.dstQueueFamilyIndex = (uint32_t)ctx->device.graphic_idx;
	barrier.image = image->handle;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags src_stage;
	VkPipelineStageFlags dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        ar_FATAL("Unsupported layout transition");
        return;
    }

    vkCmdPipelineBarrier(combuff->handle, src_stage, dst_stage, 0, 0, 0, 0, 0,
                         1, &barrier);
}

void vk_image_copy_buffer(vulkan_context_t *ctx, vulkan_image_t *image,
                          VkBuffer buffer, vulkan_commandbuffer_t *combuff) {
	(void)ctx;
	VkBufferImageCopy region;
	memory_zero(&region, sizeof(VkBufferImageCopy));

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = image->width;
	region.imageExtent.height = image->height;
	region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(combuff->handle, buffer, image->handle,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}
