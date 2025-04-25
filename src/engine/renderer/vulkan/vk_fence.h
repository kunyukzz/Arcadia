#ifndef __VULKAN_FENCE_H__
#define __VULKAN_FENCE_H__

#include "engine/renderer/vulkan/vk_type.h"

void vk_fence_init(vulkan_context_t *ctx, b8 signaling, vulkan_fence_t *fence);
void vk_fence_shut(vulkan_context_t *ctx, vulkan_fence_t *fence);
b8   vk_fence_wait(vulkan_context_t *ctx, vulkan_fence_t *fence,
                   uint64_t timeout);
void vk_fence_reset(vulkan_context_t *ctx, vulkan_fence_t *fence);

#endif //__VULKAN_FENCE_H__
