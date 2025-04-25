#ifndef __VULKAN_SWAPCHAIN_H__
#define __VULKAN_SWAPCHAIN_H__

#include "engine/renderer/vulkan/vk_type.h"

void vk_swapchain_init(vulkan_context_t *ctx, vulkan_swapchain_t *swap);
b8 vk_swapchain_reinit(vulkan_context_t *ctx, vulkan_swapchain_t *swap);
void vk_swapchain_shut(vulkan_context_t *ctx, vulkan_swapchain_t *swap);

b8   vk_swapchain_acquire_next_image(vulkan_context_t *ctx,
                                     VkSemaphore avail_sema, uint32_t *image_idx,
                                     vulkan_swapchain_t *swap);
b8   vk_swapchain_present(vulkan_context_t *ctx, VkQueue graphics_queue,
                          VkQueue present_queue, VkSemaphore   complete_semaphore,
                          uint32_t img_idx, vulkan_swapchain_t *swap);

void vk_image_view_init(vulkan_context_t *ctx, vulkan_swapchain_t *swap);
void vk_image_init(vulkan_context_t *ctx, VkImageType img_type,
                   VkFormat formats, VkImageTiling tiling,
                   VkImageUsageFlags usage, VkMemoryPropertyFlags mem_prop,
                   VkImageAspectFlags aspects, vulkan_image_t *image);
void vk_image_view_from_image(vulkan_context_t *ctx, vulkan_image_t *image,
                              VkFormat format, VkImageAspectFlags aspect_mask,
                              VkImageView *out_view);

void vk_image_view_shut(vulkan_context_t *ctx, vulkan_swapchain_t *swap);

void vk_image_shut(vulkan_context_t *ctx, vulkan_image_t *image);

#endif //__VULKAN_SWAPCHAIN_H__
