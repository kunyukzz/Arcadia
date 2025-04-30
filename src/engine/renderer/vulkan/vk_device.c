#include "engine/renderer/vulkan/vk_device.h"

#include "engine/container/dyn_array.h"
#include "engine/core/logger.h"
#include "engine/core/strings.h"

#include "engine/memory/memory.h"

typedef struct vulkan_phys_dev_req_t {
	b8 graphic;
	b8 present;
	b8 compute;
	b8 transfer;
	b8 discrete_gpu;

	const char **dev_ext_names;
} vulkan_phys_dev_req_t;

typedef struct vulkan_phys_dev_queue_family_info_t {
	uint32_t graphic_family_idx;
	uint32_t present_family_idx;
	uint32_t compute_family_idx;
	uint32_t transfer_family_idx;
} vulkan_phys_dev_queue_family_info_t;


/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */

b8 query_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface,
                        const vulkan_phys_dev_req_t *req,
                        vulkan_phys_dev_queue_family_info_t *out_info) {
	out_info->graphic_family_idx = (uint32_t)-1;
	out_info->present_family_idx = (uint32_t)-1;
	out_info->transfer_family_idx = (uint32_t)-1;
	out_info->compute_family_idx = (uint32_t)-1;

	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);
	VkQueueFamilyProperties props[32];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props);

	uint8_t min_score = 255;

	for (uint32_t i = 0; i < count; ++i) {
		uint8_t score = 0;

		if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			out_info->graphic_family_idx = i;
			++score;
		}

		if (props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			out_info->compute_family_idx = i;
			++score;
		}

		if ((props[i].queueFlags & VK_QUEUE_TRANSFER_BIT)) {
			if (score <= min_score) {
				min_score = score;
				out_info->transfer_family_idx = i;
			}
		}

		VkBool32 present_supported = VK_FALSE;
		VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
				&present_supported));

		if (present_supported)
			out_info->present_family_idx = i;
	}

	//ar_INFO("Device meets queue requirement");
    //ar_DEBUG("Graphics Family Index: %i", out_info->graphic_family_idx);
    //ar_DEBUG("Present Family Index: %i", out_info->present_family_idx);
    //ar_DEBUG("Compute Family Index: %i", out_info->compute_family_idx);
    //ar_DEBUG("Transfer Family Index: %i", out_info->transfer_family_idx);

	return (!req->graphic || (out_info->graphic_family_idx != (uint32_t)-1)) &&
		   (!req->present || (out_info->present_family_idx != (uint32_t)-1)) &&
		   (!req->compute || (out_info->compute_family_idx != (uint32_t)-1)) &&
		   (!req->transfer || (out_info->transfer_family_idx != (uint32_t)-1));
}

b8 check_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface,
                           vulkan_swapchain_support_t *out_support) {
	vk_device_query_swapchain(device, surface, out_support);

	if (out_support->format_count < 1 || out_support->present_mode_count < 1) {
		if (out_support->formats) {
			memory_free(out_support->formats,
						sizeof(VkSurfaceFormatKHR) * 
						out_support->format_count,
						MEMTAG_RENDERER);
		}
		if (out_support->present_mode) {
			memory_free(out_support->present_mode,
						sizeof(VkPresentModeKHR) * 
						out_support->present_mode_count,
						MEMTAG_RENDERER);
		}
		return false;
	}
	return true;
}

b8 check_device_extension(VkPhysicalDevice device, const char **req_ext,
                          uint32_t req_count) {
	uint32_t avail_count = 0;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &avail_count, 0));
	if (avail_count == 0) return false;

	VkExtensionProperties *avail = memory_alloc(
		sizeof(VkExtensionProperties) * avail_count, MEMTAG_RENDERER);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &avail_count,
												  avail));

    for (uint32_t i = 0; i < req_count; ++i) {
		b8 found = false;
		for (uint32_t j = 0; j < avail_count; ++j) {
			if (string_equal(req_ext[i], avail[j].extensionName)) {
				found = true;
				break;
			}
		}

		if (!found) {
			ar_INFO("Required extension missing '%s'", req_ext[i]);
			memory_free(avail,
						sizeof(VkExtensionProperties) * avail_count,
						MEMTAG_RENDERER);
			return false;
		}
	}

	memory_free(avail, sizeof(VkExtensionProperties) * avail_count,
				MEMTAG_RENDERER);

	return true;
}


b8 phys_dev_meet_req(VkPhysicalDevice device,
					 VkSurfaceKHR surface,
					 const VkPhysicalDeviceProperties* properties,
					 const VkPhysicalDeviceFeatures* features,
					 const vulkan_phys_dev_req_t *req,
					 vulkan_phys_dev_queue_family_info_t *queue_family_info,
					 vulkan_swapchain_support_t *swapchain_support) {
	(void)features;
	if (req->discrete_gpu) {
		if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			ar_INFO("Not discrete gpu. Skip..");
			return false;
		}
	}

	if (!query_queue_families(device, surface, req, queue_family_info)) {
		ar_WARNING("Queue family requirements not met");
		return false;
	}


	if (!check_swapchain_support(device, surface, swapchain_support)) {
		ar_WARNING("Swapchain support insufficient");
		return false;
	}

	if (req->dev_ext_names &&
		!check_device_extension(device, req->dev_ext_names,
								dyn_array_length(req->dev_ext_names))) {
		ar_WARNING("Required device extensions not available");
		return false;
	}

	dyn_array_destroy(req->dev_ext_names);
	return true;
}

b8 select_phys_dev(vulkan_context_t *context) {
	uint32_t phys_dev_count = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &phys_dev_count, 0));
	if (phys_dev_count == 0) {
		ar_FATAL("No devices support Vulkan were found");
	}

	VkPhysicalDevice phys_dev[1];
	VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &phys_dev_count,
										phys_dev));
	for (uint32_t i = 0; i < phys_dev_count; ++i) {
		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(phys_dev[i], &prop);

		VkPhysicalDeviceFeatures feats;
		vkGetPhysicalDeviceFeatures(phys_dev[i], &feats);

		VkPhysicalDeviceMemoryProperties memory;
		vkGetPhysicalDeviceMemoryProperties(phys_dev[i], &memory);

		b8 support_dev_local_host_vsb = false;
		for (uint32_t j = 0; j < memory.memoryTypeCount; ++j) {
			if (
				((memory.memoryTypes[j].propertyFlags &
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
				((memory.memoryTypes[j].propertyFlags &
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)) {
				support_dev_local_host_vsb = true;
				break;
			}
		}

		vulkan_phys_dev_req_t requirements = {};
		requirements.graphic = true;
		requirements.present = true;
		requirements.transfer = true;
		requirements.discrete_gpu = false;
		requirements.dev_ext_names = dyn_array_create(const char *);
		dyn_array_push(requirements.dev_ext_names,
					   &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		vulkan_phys_dev_queue_family_info_t qinfo = {};
		b8 result = phys_dev_meet_req(phys_dev[i], context->surface,
									  &prop, &feats, &requirements,
									  &qinfo, &context->device.support);

		if (result) {
			ar_INFO("Selected Device: '%s'", prop.deviceName);
			switch (prop.deviceType) {
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:
				ar_INFO("GPU in unknown.");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				ar_INFO("GPU Type is Integrated.");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				ar_INFO("GPU Type is Discrete.");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				ar_INFO("GPU Type is Virtual GPU");
				break;
			default: break;
			}

			ar_INFO("GPU Driver Version: %d.%d.%d", 
					VK_VERSION_MAJOR(prop.driverVersion),
					VK_VERSION_MINOR(prop.driverVersion),
					VK_VERSION_PATCH(prop.driverVersion));

			ar_INFO("Vulkan API Version: %d.%d.%d",
					VK_VERSION_MAJOR(prop.apiVersion),
					VK_VERSION_MINOR(prop.apiVersion),
					VK_VERSION_PATCH(prop.apiVersion));

			for (uint32_t k = 0; k < memory.memoryHeapCount; ++k) {
				float mem_size = (((float)memory.memoryHeaps[k].size) /
									1024.0f / 1024.0f / 1024.0f);

				if (memory.memoryHeaps[k].flags &
					VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
					ar_INFO("GPU Memory: %.2f GiB", mem_size);
				} else {
					ar_INFO("Shared system memory: %.2f GiB", mem_size); 
				}
			}

			context->device.phys_dev = phys_dev[i];
			context->device.graphic_idx = (int32_t)qinfo.graphic_family_idx;
			context->device.present_idx = (int32_t)qinfo.present_family_idx;
			context->device.transfer_idx = (int32_t)qinfo.transfer_family_idx;

			context->device.properties = prop;
			context->device.features = feats;
			context->device.memory = memory;
            context->device.support_dev_local_host_vsb =
                support_dev_local_host_vsb;
            break;
        }
    }


	if (!context->device.phys_dev) {
		ar_ERROR("No physical device found meet requirements");
		return false;
	}

	return true;
}

/* ========================================================================== */
/* ========================================================================== */

b8 vk_device_init(vulkan_context_t *context) {
	if (!select_phys_dev(context))
		return false;

	//ar_INFO("Create logical device...");

	b8 present_graphic_queue =
		context->device.graphic_idx == context->device.present_idx;
	b8 transfer_graphic_queue =
		context->device.graphic_idx == context->device.transfer_idx;
	uint32_t idx_count = 1;
	if (!present_graphic_queue)
		idx_count++;
	if (!transfer_graphic_queue)
		idx_count++;
	uint32_t indices[32];
	uint8_t idx = 0;
	indices[idx++] = (uint32_t)context->device.graphic_idx;

	if (!present_graphic_queue)
		indices[idx++] = (uint32_t)context->device.present_idx;
	if (!transfer_graphic_queue)
		indices[idx++] = (uint32_t)context->device.transfer_idx;

	VkDeviceQueueCreateInfo q_infos[32];
	for (uint32_t i = 0; i < idx_count; ++i) {
		q_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		q_infos[i].queueFamilyIndex = indices[i];
		q_infos[i].queueCount = 1;
		float q_prior = 1.0f;
		q_infos[i].pQueuePriorities = &q_prior;
	}

	//TODO: implement this later
	VkPhysicalDeviceFeatures dev_feats = {};

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = idx_count;
	create_info.pQueueCreateInfos = q_infos;
	create_info.pEnabledFeatures = &dev_feats;
	create_info.enabledExtensionCount = 1;
	const char *ext_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	create_info.ppEnabledExtensionNames = &ext_names;

	/* Create Device */
	VK_CHECK(vkCreateDevice(context->device.phys_dev, &create_info,
							context->alloc, &context->device.logic_dev));
	ar_DEBUG("Vulkan Logical Device Created");

	/* Device Get Queue */
	vkGetDeviceQueue(context->device.logic_dev,
					 (uint32_t)context->device.graphic_idx, 0,
					 &context->device.graphics_queue);
	vkGetDeviceQueue(context->device.logic_dev,
					 (uint32_t)context->device.present_idx, 0,
					 &context->device.present_queue);
	vkGetDeviceQueue(context->device.logic_dev,
					 (uint32_t)context->device.transfer_idx, 0,
					 &context->device.transfer_queue);

	/* Command pool for graphics queue */
	VkCommandPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.queueFamilyIndex = (uint32_t)context->device.graphic_idx;
	pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK(vkCreateCommandPool(context->device.logic_dev,
								 &pool_create_info, context->alloc,
								 &context->device.graphics_command_pool));
	ar_DEBUG("Vulkan Commmand Pool Created");
	return true;
}

void vk_device_shut(vulkan_context_t *context) {
	context->device.graphics_queue = 0;
	context->device.present_queue = 0;
	context->device.transfer_queue = 0;

	vkDestroyCommandPool(context->device.logic_dev,
						 context->device.graphics_command_pool,
						 context->alloc);

	if (context->device.logic_dev) {
		vkDestroyDevice(context->device.logic_dev, context->alloc);
		context->device.logic_dev = 0;
		ar_INFO("Kill Logical Device");
	}

	ar_INFO("Kill Physical Device");
	context->device.phys_dev = 0;

	if (context->device.support.formats) {
		memory_free(context->device.support.formats, sizeof(VkSurfaceFormatKHR) *
					  context->device.support.format_count, MEMTAG_RENDERER);
	  context->device.support.formats = 0;
	  context->device.support.format_count = 0;
	}

	if (context->device.support.present_mode) {
		memory_free(context->device.support.present_mode,
					sizeof(VkPresentModeKHR) * 
					context->device.support.present_mode_count,
					MEMTAG_RENDERER);

		context->device.support.present_mode = 0;
		context->device.support.present_mode_count = 0;
	}

	memory_zero(&context->device.support.capable,
				sizeof(context->device.support.capable));

	context->device.graphic_idx = -1;
	context->device.present_idx = -1;
	context->device.transfer_idx = -1;
}

void vk_device_query_swapchain(VkPhysicalDevice device, VkSurfaceKHR surface,
                               vulkan_swapchain_support_t *support_info) {
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
			 device, surface, &support_info->capable));
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
			 device, surface, &support_info->format_count, 0));

	if (support_info->format_count != 0) {
		if (!support_info->formats) {
			support_info->formats =
				memory_alloc(sizeof(VkSurfaceFormatKHR) *
				support_info->format_count,
                MEMTAG_RENDERER);	
		}

		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
				 device, surface, &support_info->format_count,
				 support_info->formats));
	}

	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
			 device, surface, &support_info->present_mode_count, 0));

	if (support_info->present_mode_count != 0) {
		if (!support_info->present_mode) {
			support_info->present_mode = 
				memory_alloc(sizeof(VkPresentModeKHR) * 
				support_info->present_mode_count, MEMTAG_RENDERER);
		}

		VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
				 device, surface, &support_info->present_mode_count,
				 support_info->present_mode));
	}
}

b8 vk_device_detect_depth(vulkan_device_t *device) {
	const uint64_t candidate_count = 3;
	VkFormat candidates[3] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT};

	uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	for (uint64_t i = 0; i < candidate_count; ++i) {
		VkFormatProperties prop;
		vkGetPhysicalDeviceFormatProperties(device->phys_dev, candidates[i], &prop);

		if ((prop.linearTilingFeatures & flags) == flags) {
			device->depth_format = candidates[i];
			return true;
		} else if ((prop.optimalTilingFeatures & flags) == flags) {
			device->depth_format = candidates[i];
			return true;
		}
	}
	return false;
}
