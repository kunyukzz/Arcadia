#include "engine/renderer/vulkan/vk_buffer.h"

#include "engine/container/free_list.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"
#include "engine/renderer/vulkan/vk_combuffer.h"


b8 vk_buffer_init(vulkan_context_t *ctx, uint64_t size,
                  VkBufferUsageFlagBits usage, uint32_t mem_prop_flag,
                  b8 bind_on_create, b8 use_freelist, vulkan_buffer_t *buffer) {
    memory_zero(buffer, sizeof(vulkan_buffer_t));
    buffer->total_size           = size;
    buffer->usage                = usage;
    buffer->mem_prop_flag        = mem_prop_flag;
    buffer->has_freelist         = use_freelist;

	if (use_freelist) {
		buffer->freelist_mem_req = 0;
		freelist_init(size, &buffer->freelist_mem_req, 0, 0);
        buffer->freelist_block =
            memory_alloc(buffer->freelist_mem_req, MEMTAG_RENDERER);
        freelist_init(size, &buffer->freelist_mem_req, buffer->freelist_block,
                      &buffer->buffer_freelist);
    }

    VkBufferCreateInfo buff_info = {};
    buff_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_info.size               = size;
    buff_info.usage              = usage;
    buff_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateBuffer(ctx->device.logic_dev, &buff_info, ctx->alloc,
                            &buffer->handle));

    /* Get Memory Requirements */
    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(ctx->device.logic_dev, buffer->handle,
                                  &mem_req);
    buffer->mem_idx =
        ctx->find_mem_idx(mem_req.memoryTypeBits, buffer->mem_prop_flag);
    if (buffer->mem_idx == -1) {
        ar_ERROR(
            "Unable to create buffer due to require memory index not found");
        return false;
    }

    /* Set Memory Info */
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize       = mem_req.size;
    alloc_info.memoryTypeIndex      = (uint32_t)buffer->mem_idx;

    /* Allocate Memory */
    VkResult result = vkAllocateMemory(ctx->device.logic_dev, &alloc_info,
                                       ctx->alloc, &buffer->memory);
    if (result != VK_SUCCESS) {
        ar_ERROR("Unable to create vulkan buffer due to memory failed. Error: "
                 "%i", result);
        return false;
    }

    if (bind_on_create) {
        vk_buffer_bind(ctx, buffer, 0);
    }

    return true;
}

void  vk_buffer_shut(vulkan_context_t *ctx, vulkan_buffer_t *buffer) {
	if (buffer->memory) {
		vkFreeMemory(ctx->device.logic_dev, buffer->memory, ctx->alloc);
		buffer->memory = 0;
	}

	if (buffer->handle) {
		vkDestroyBuffer(ctx->device.logic_dev, buffer->handle, ctx->alloc);
		buffer->handle = 0;
	}

    buffer->total_size = 0;
    buffer->usage      = 0;
    buffer->is_locked  = false;
}

b8 vk_buffer_resize(vulkan_context_t *ctx, uint64_t new_size,
                    vulkan_buffer_t *buffer, VkQueue queue,
                    VkCommandPool pool) {
	if (new_size < buffer->total_size) {
		ar_ERROR("vk_buffer_resize - require new size be larger than old size.");
		return false;
	}

	/* Resize freelist */
	if (buffer->has_freelist) {
		uint64_t new_mem_req = 0;
		freelist_resize(&buffer->buffer_freelist, &new_mem_req, 0, 0, 0);
		void *new_block = memory_alloc(new_mem_req, MEMTAG_RENDERER);
		void *old_block = 0;
        if (!freelist_resize(&buffer->buffer_freelist, &new_mem_req, new_block,
                             new_size, &old_block)) {
            ar_ERROR("vk_buffer_resize - failed to resize internal freelist");
            memory_free(new_block, new_mem_req, MEMTAG_RENDERER);
            return false;
        }

		memory_free(old_block, buffer->freelist_mem_req, MEMTAG_RENDERER);
		buffer->freelist_mem_req = new_mem_req;
		buffer->freelist_block = new_block;
    }
	buffer->total_size = new_size;

    VkBufferCreateInfo buff_info = {};
    buff_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_info.size               = new_size;
    buff_info.usage              = buffer->usage;
    buff_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer new_buffer;
    VK_CHECK(vkCreateBuffer(ctx->device.logic_dev, &buff_info, ctx->alloc,
                            &new_buffer));

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(ctx->device.logic_dev, new_buffer, &mem_req);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize       = mem_req.size;
    alloc_info.memoryTypeIndex      = (uint32_t)buffer->mem_idx;

    VkDeviceMemory new_mem;
    VkResult       result = vkAllocateMemory(ctx->device.logic_dev, &alloc_info,
                                             ctx->alloc, &new_mem);
    if (result != VK_SUCCESS) {
        ar_ERROR("Unable ro resize buffer due to memory Allocate failed. "
                 "Error: %i",
                 result);
        return false;
    }

    VK_CHECK(vkBindBufferMemory(ctx->device.logic_dev, new_buffer, new_mem, 0));

    vk_buffer_copy(ctx, pool, 0, queue, buffer->handle, 0, new_buffer, 0,
                   buffer->total_size);
    vkDeviceWaitIdle(ctx->device.logic_dev);

    /* Destroy Old Buffer */
    vk_buffer_shut(ctx, buffer);

    return true;
}

void vk_buffer_bind(vulkan_context_t *ctx, vulkan_buffer_t *buffer,
                    uint64_t offset) {
    VK_CHECK(vkBindBufferMemory(ctx->device.logic_dev, buffer->handle,
                                buffer->memory, offset));
}

void *vk_buffer_lock_mem(vulkan_context_t *ctx, vulkan_buffer_t *buffer,
                         uint64_t offset, uint64_t size, uint32_t flag) {
	void *data;
    VK_CHECK(vkMapMemory(ctx->device.logic_dev, buffer->memory, offset, size,
                         flag, &data));
    return data;
}

void  vk_buffer_unlock_mem(vulkan_context_t *ctx, vulkan_buffer_t *buffer) {
	vkUnmapMemory(ctx->device.logic_dev, buffer->memory);
}

b8 vk_buffer_allocate(vulkan_buffer_t *buffer, uint64_t size,
                      uint64_t *offset) {
    if (!buffer || !size || !offset) {
        ar_ERROR("vk_buffer_allocate requires valid buffer, a nonzero size and "
                 "valid pointer to hold offset.");
        return false;
    }

    if (!buffer->has_freelist) {
        ar_WARNING(
            "vk_buffer_allocate called on a buffer not using freelists. Offset "
            "will not be valid. Call vulkan_buffer_load_data instead.");
        *offset = 0;
        return true;
    }
    return freelist_block_alloc(&buffer->buffer_freelist, size, offset);
}

b8 vk_buffer_free(vulkan_buffer_t *buffer, uint64_t size, uint64_t offset) {
    if (!buffer || !size) {
        ar_ERROR("vk_buffer_free requires valid buffer and a nonzero size.");
        return false;
    }

    if (!buffer->has_freelist) {
        ar_WARNING("vk_buffer_allocate called on a buffer not using "
                   "freelists. Nothing was done.");
        return true;
    }
    return freelist_block_free(&buffer->buffer_freelist, size, offset);
}

void  vk_buffer_load_data(vulkan_context_t *ctx, vulkan_buffer_t *buffer,
                          uint64_t offset, uint64_t size, uint32_t flag,
                          const void *data) {
	void *data_ptr;
    VK_CHECK(vkMapMemory(ctx->device.logic_dev, buffer->memory, offset, size,
                         flag, &data_ptr));
    memory_copy(data_ptr, data, size);
    vkUnmapMemory(ctx->device.logic_dev, buffer->memory);
}

void  vk_buffer_copy(vulkan_context_t *ctx, VkCommandPool pool, VkFence fence,
                     VkQueue queue, VkBuffer source, uint64_t source_offset,
                     VkBuffer dest, uint64_t dest_offset, uint64_t size) {
	(void)fence;
	vkQueueWaitIdle(queue);
	vkDeviceWaitIdle(ctx->device.logic_dev);
	
	vulkan_commandbuffer_t temp;
	vk_combuff_single_use_init(ctx, pool, &temp);

	VkBufferCopy copy;
	copy.srcOffset = source_offset;
	copy.dstOffset = dest_offset;
	copy.size = size;

	vkCmdCopyBuffer(temp.handle, source, dest, 1, &copy);

	vk_combuff_single_use_shut(ctx, pool, &temp, queue);
}


