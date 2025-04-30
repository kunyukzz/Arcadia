#include "engine/renderer/vulkan/vk_combuffer.h"

#include "engine/memory/memory.h"

void vk_combuff_init(vulkan_context_t *ctx, VkCommandPool pool,
                     vulkan_commandbuffer_t *combuff) {
	memory_zero(combuff, sizeof(*combuff));

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;

	combuff->state = _STATE_NOT_ALLOCATED;
    VK_CHECK(vkAllocateCommandBuffers(ctx->device.logic_dev, &alloc_info,
                                      &combuff->handle));
	combuff->state = _STATE_READY;
}

void vk_combuff_begin(vulkan_commandbuffer_t *combuff) {
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(vkBeginCommandBuffer(combuff->handle, &begin_info));
	combuff->state = _STATE_RECORDING;
}

void vk_combuff_end(vulkan_commandbuffer_t *combuff) {
	VK_CHECK(vkEndCommandBuffer(combuff->handle));
	combuff->state = _STATE_RECORD_END;
}

void vk_combuff_shut(vulkan_context_t *ctx, VkCommandPool pool,
                     vulkan_commandbuffer_t *combuff) {
    vkFreeCommandBuffers(ctx->device.logic_dev, pool, 1, &combuff->handle);
    combuff->handle = 0;
    combuff->state  = _STATE_NOT_ALLOCATED;
}

void vk_combuff_update(vulkan_commandbuffer_t *combuff) {
	combuff->state = _STATE_SUBMITTED;
}

void vk_combuff_reset(vulkan_commandbuffer_t *combuff) {
	combuff->state = _STATE_READY;
}

void vk_combuff_single_use_init(vulkan_context_t *ctx, VkCommandPool pool,
                                vulkan_commandbuffer_t *combuff) {
	vk_combuff_init(ctx, pool, combuff);
	vk_combuff_begin(combuff);
}

void vk_combuff_single_use_shut(vulkan_context_t *ctx, VkCommandPool pool,
                                vulkan_commandbuffer_t *combuff,
                                VkQueue                 queues) {
	vk_combuff_end(combuff);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &combuff->handle;

	VK_CHECK(vkQueueSubmit(queues, 1, &submit_info, 0));
	VK_CHECK(vkQueueWaitIdle(queues));

	vk_combuff_shut(ctx, pool, combuff);
}
