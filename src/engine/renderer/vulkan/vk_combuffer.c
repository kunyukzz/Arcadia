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

    VK_CHECK(vkAllocateCommandBuffers(ctx->device.logic_dev, &alloc_info,
                                      &combuff->handle));
}

void vk_combuff_begin(vulkan_commandbuffer_t *combuff) {
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(vkBeginCommandBuffer(combuff->handle, &begin_info));
}

void vk_combuff_end(vulkan_commandbuffer_t *combuff) {
	VK_CHECK(vkEndCommandBuffer(combuff->handle));
}

void vk_combuff_shut(vulkan_context_t *ctx, VkCommandPool pool, vulkan_commandbuffer_t *combuff) {
	vkFreeCommandBuffers(ctx->device.logic_dev, pool, 1, &combuff->handle);
	combuff->handle = 0;
}

