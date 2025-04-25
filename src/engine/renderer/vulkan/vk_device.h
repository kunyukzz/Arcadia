#ifndef __VK_DEVICE_H__
#define __VK_DEVICE_H__

#include "engine/renderer/vulkan/vk_type.h"

b8 vk_device_init(vulkan_context_t *context);
void vk_device_shut(vulkan_context_t *context);
void vk_device_query_swapchain(VkPhysicalDevice device, VkSurfaceKHR surface,
                               vulkan_swapchain_support_t *support_info);
b8 vk_device_detect_depth(vulkan_device_t *device);

#endif //__VK_DEVICE_H__
