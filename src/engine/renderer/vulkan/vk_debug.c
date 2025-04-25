#include "engine/renderer/vulkan/vk_debug.h"
#include "engine/renderer/vulkan/vk_type.h"

#include "engine/core/logger.h"

static VkDebugUtilsMessengerEXT debug_messenger = 0;

b8 vk_debug_init(struct vulkan_context_t *context) {
	uint32_t log = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
				   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

	VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
	debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_info.messageSeverity = log;
	debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

	debug_info.pfnUserCallback = vk_debug_callback;

	PFN_vkCreateDebugUtilsMessengerEXT f =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			context->instance, "vkCreateDebugUtilsMessengerEXT");
    ar_assert_msg(f, "Failed to create Debug Messenger");
	VK_CHECK(f(context->instance, &debug_info, context->alloc, &debug_messenger));

	return true;
}

void vk_debug_shut(struct vulkan_context_t *context) {
	if (debug_messenger) {
		PFN_vkDestroyDebugUtilsMessengerEXT f =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
				context->instance, "vkDestroyDebugUtilsMessengerEXT");

		f(context->instance, debug_messenger, context->alloc);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
    VkDebugUtilsMessageTypeFlagsEXT msg_type,
    const VkDebugUtilsMessengerCallbackDataEXT *_callback, void *_data) {
	(void)msg_type;
	(void)_data;

    switch (msg_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
          ar_ERROR(_callback->pMessage);
          break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
          ar_WARNING(_callback->pMessage);
          break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
          //ar_TRACE(_callback->pMessage);
          break;
        default: break;
        }
	return false;
}

