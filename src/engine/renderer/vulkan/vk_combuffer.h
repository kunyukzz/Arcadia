#ifndef __VULKAN_COMMANDBUFFER_H__
#define __VULKAN_COMMANDBUFFER_H__

#include "engine/renderer/vulkan/vk_type.h"

void vk_combuff_init(vulkan_context_t *ctx, VkCommandPool pool,
                     vulkan_commandbuffer_t *combuff);

void vk_combuff_begin(vulkan_commandbuffer_t *combuff);
void vk_combuff_end(vulkan_commandbuffer_t *combuff);
void vk_combuff_shut(vulkan_context_t *ctx, VkCommandPool pool, vulkan_commandbuffer_t *combuff);

#endif //__VULKAN_COMMANDBUFFER_H__
