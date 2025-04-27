#ifndef __VULKAN_BUFFER_H__
#define __VULKAN_BUFFER_H__

#include "engine/renderer/vulkan/vk_type.h"

b8    vk_buffer_init(vulkan_context_t *ctx, uint64_t size,
                     VkBufferUsageFlagBits usage, uint32_t mem_prop_flag,
                     b8 bind_on_create, vulkan_buffer_t *buffer);

void  vk_buffer_shut(vulkan_context_t *ctx, vulkan_buffer_t *buffer);

b8    vk_buffer_resize(vulkan_context_t *ctx, uint64_t new_size,
                       vulkan_buffer_t *buffer, VkQueue queue, VkCommandPool pool);

void  vk_buffer_bind(vulkan_context_t *ctx, vulkan_buffer_t *buffer,
                     uint64_t offset);

void *vk_buffer_lock_mem(vulkan_context_t *ctx, vulkan_buffer_t *buffer,
                         uint64_t offset, uint64_t size, uint32_t flag);

void  vk_buffer_unlock_mem(vulkan_context_t *ctx, vulkan_buffer_t *buffer);

void  vk_buffer_load_data(vulkan_context_t *ctx, vulkan_buffer_t *buffer,
                          uint64_t offset, uint64_t size, uint32_t flag,
                          const void *data);

void  vk_buffer_copy(vulkan_context_t *ctx, VkCommandPool pool, VkFence fence,
                     VkQueue queue, VkBuffer source, uint64_t source_offset,
                     VkBuffer dest, uint64_t dest_offset, uint64_t size);

#endif //__VULKAN_BUFFER_H__
