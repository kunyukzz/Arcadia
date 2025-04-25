#ifndef __VULKAN_RENDERPASS_H__
#define __VULKAN_RENDERPASS_H__

#include "engine/renderer/vulkan/vk_type.h"

void vk_renderpass_init(vulkan_context_t *ctx, vulkan_renderpass_t *renderpass);

void vk_renderpass_begin(vulkan_commandbuffer_t *combuff,
                         vulkan_renderpass_t    *renderpass,
                         VkFramebuffer framebuffer);

void vk_renderpass_end(vulkan_commandbuffer_t *combuff);

void vk_renderpass_shut(vulkan_context_t *ctx, vulkan_renderpass_t *renderpass);

#endif //__VULKAN_RENDERPASS_H__
