#ifndef __VULKAN_IMAGE_H__
#define __VULKAN_IMAGE_H__

#include "engine/renderer/vulkan/vk_type.h"

void vk_image_transition_layout(vulkan_context_t       *ctx,
                                vulkan_commandbuffer_t *combuff,
                                vulkan_image_t *image, VkFormat *format,
                                VkImageLayout old_layout,
                                VkImageLayout new_layout);

void vk_image_copy_buffer(vulkan_context_t *ctx, vulkan_image_t *image,
                          VkBuffer buffer, vulkan_commandbuffer_t *combuff);

#endif //__VULKAN_IMAGE_H__
