#ifndef __VULKAN_FRAMEBUFFER_H__
#define __VULKAN_FRAMEBUFFER_H__

#include "engine/renderer/vulkan/vk_type.h"

void vk_framebuffer_init(vulkan_context_t *ctx, vulkan_renderpass_t *renderpass,
                         VkExtent2D extents, VkImageView *attach,
                         uint32_t attach_count, vulkan_framebuffer_t *fbuffer);

void vk_framebuffer_shut(vulkan_context_t *ctx, vulkan_framebuffer_t *fbuffer);

#endif //__VULKAN_FRAMEBUFFER_H__
