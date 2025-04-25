#ifndef __VK_DEBUG_H__
#define __VK_DEBUG_H__

#include "engine/renderer/vulkan/vk_type.h"
#include <vulkan/vulkan.h>

b8 vk_debug_init(struct vulkan_context_t *context);
void vk_debug_shut(struct vulkan_context_t *context);
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
    VkDebugUtilsMessageTypeFlagBitsEXT msg_type,
    const VkDebugUtilsMessengerCallbackDataEXT *_callback, void *_data);

#endif //__VK_DEBUG_H__
