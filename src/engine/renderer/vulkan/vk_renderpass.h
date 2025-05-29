#ifndef __VULKAN_RENDERPASS_H__
#define __VULKAN_RENDERPASS_H__

#include "engine/renderer/vulkan/vk_type.h"

typedef enum renderpass_clear_flag_t {
	CLEAR_NON_FLAG = 0x0,
	CLEAR_COLOR_BUFFER = 0x1,
	CLEAR_DEPTH_BUFFER = 0x2,
	CLEAR_STENCIL_BUFFER = 0x4
} renderpass_clear_flag_t;

void vk_renderpass_init(vulkan_context_t *ctx, vec4 render_area,
                        vec4 clear_color, float depth, uint32_t stencil,
                        uint8_t clear_flags, b8 has_prev_pass, b8 has_next_pass,
                        vulkan_renderpass_t *renderpass);

void vk_renderpass_begin(vulkan_commandbuffer_t *combuff,
                         vulkan_renderpass_t    *renderpass,
                         VkFramebuffer framebuffer);

void vk_renderpass_end(vulkan_commandbuffer_t *combuff, vulkan_renderpass_t *renderpass);

void vk_renderpass_shut(vulkan_context_t *ctx, vulkan_renderpass_t *renderpass);

#endif //__VULKAN_RENDERPASS_H__
