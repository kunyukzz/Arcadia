#ifndef __VK_UTILS_H__
#define __VK_UTILS_H__

#include "engine/renderer/vulkan/vk_type.h"
#include "engine/container/dyn_array.h"
#include "engine/core/logger.h"
#include "engine/core/strings.h"

static inline const char **vk_req_get_extensions(uint32_t *count) {
	const char **extensions = dyn_array_create(const char *);
	dyn_array_push(extensions, &VK_KHR_SURFACE_EXTENSION_NAME);

#if OS_LINUX
	dyn_array_push(extensions, &"VK_KHR_xcb_surface");
#elif OS_WINDOWS
	dyn_array_push(extensions, &"VK_KHR_win32_surface");
#endif
	
#if defined (_DEBUG)
	ar_DEBUG("Require Extensions:");
	dyn_array_push(extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	uint32_t l = dyn_array_length(extensions);
	for (uint32_t i = 0; i < l; ++i) {
        ar_DEBUG(extensions[i]);
    }
#endif	

	*count = dyn_array_length(extensions);
	return extensions;
}

static inline const char **vk_get_validation_layers(uint32_t *count) {
#if defined (_DEBUG)
	ar_INFO("Validation layer enabled. Enumerating...");
	const char **layers = dyn_array_create(const char *);
	dyn_array_push(layers, &"VK_LAYER_KHRONOS_validation");
	*count = dyn_array_length(layers);

	uint32_t avail_count = 0;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&avail_count, 0));
	VkLayerProperties *avail_layer = dyn_array_reserved(VkLayerProperties, avail_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&avail_count, avail_layer));

	for (uint32_t i = 0; i < *count; ++i) {
		b8 found = false;
		for (uint32_t j = 0; j < avail_count; ++j) {
			if (string_equal(layers[i], avail_layer[j].layerName)) {
				found = true;
				ar_TRACE("Found!");
				break;
			}
		}
		if (!found) {
			ar_FATAL("Missing required validation layer: %s", layers[i]);
			return NULL;
		}
	}

	ar_INFO("Required Validation Layers are present.");

	dyn_array_destroy(avail_layer);
	return layers;
#endif

	*count = 0;
	return NULL;
}

#endif //__VK_UTILS_H__
