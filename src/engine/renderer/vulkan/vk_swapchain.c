#include "engine/renderer/vulkan/vk_swapchain.h"
#include "engine/renderer/vulkan/vk_device.h"

#include "engine/core/logger.h"
#include "engine/memory/memory.h"

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */

void vk_internal_swapchain(vulkan_context_t *ctx, vulkan_swapchain_t *swap,
                           VkSwapchainKHR oldswap) {
	/* Get Device Capabilities */
    vk_device_query_swapchain(ctx->device.phys_dev, ctx->surface,
                              &ctx->device.support);

	/* Set Surface Format */
	for (uint32_t i = 0; i < ctx->device.support.format_count; ++i) {
		VkSurfaceFormatKHR format = ctx->device.support.formats[i];

        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swap->image_format = format;
            break;
        }
    }

	/* Set Extent Swapchain */
    if (ctx->device.support.capable.currentExtent.width != UINT32_MAX) {
		swap->extents = ctx->device.support.capable.currentExtent;
	} else {
		uint32_t width = ctx->framebuffer_w;
    	uint32_t height = ctx->framebuffer_h;

		VkExtent2D true_ext = {width, height};
		VkExtent2D min = ctx->device.support.capable.minImageExtent;
		VkExtent2D max = ctx->device.support.capable.maxImageExtent;

		true_ext.width = ar_CLAMP(true_ext.width, min.width, max.width);
		true_ext.height = ar_CLAMP(true_ext.height, min.height, max.height);

		swap->extents = true_ext;
	}

	/* Set Image Count */
	uint32_t   img_count = ctx->device.support.capable.minImageCount + 1;
    if (ctx->device.support.capable.maxImageCount > 0 &&
        img_count > ctx->device.support.capable.maxImageCount) {
        img_count = ctx->device.support.capable.maxImageCount;
    }

	swap->max_frame_in_flight = img_count - 1;
    ar_TRACE("Swapchain Image Count: %u, Max Fame in Flight: %u", img_count,
            swap->max_frame_in_flight);

    /* Set Present Mode */
	VkPresentModeKHR present   = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < ctx->device.support.present_mode_count; ++i) {
        VkPresentModeKHR mode = ctx->device.support.present_mode[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present = mode;
            break;
        }
    }

    switch (present) {
    case VK_PRESENT_MODE_FIFO_KHR:

        ar_TRACE("Present Mode: VK_PRESENT_MODE_FIFO_KHR (V-Sync)");
        break;
    case VK_PRESENT_MODE_MAILBOX_KHR:
        ar_TRACE("Present Mode: VK_PRESENT_MODE_MAILBOX_KHR (Triple Buffer)");
        break;
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        ar_TRACE("Present Mode: VK_PRESENT_MODE_IMMEDIATE_KHR (No V-Sync)");
        break;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        ar_TRACE("Present Mode: VK_PRESENT_MODE_FIFO_RELAXED_KHR");
        break;
    default:
        break;
    }

    if (oldswap && oldswap != VK_NULL_HANDLE) {
        ar_TRACE("Recreate swapchain with oldSwapchain handle: %p",
                 (void *)oldswap);
    } else {
        ar_TRACE("Create swapchain without oldSwapchain (first-time or full "
                 "reinit)");
    }

    /* Create Swapchain */
    VkSwapchainCreateInfoKHR swap_info = {};
    swap_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_info.surface          = ctx->surface;
    swap_info.minImageCount    = img_count;
    swap_info.imageFormat      = swap->image_format.format;
    swap_info.imageExtent      = swap->extents;
    swap_info.imageColorSpace  = swap->image_format.colorSpace;
    swap_info.imageArrayLayers = 1;
    swap_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_info.preTransform     = ctx->device.support.capable.currentTransform;
    swap_info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_info.presentMode      = present;
    swap_info.clipped          = VK_TRUE;
    swap_info.oldSwapchain     = oldswap;

	VK_CHECK(vkCreateSwapchainKHR(ctx->device.logic_dev, &swap_info,
								  ctx->alloc, &ctx->swapchain.handle));

	vk_device_detect_depth(&ctx->device);

	if (swap->image_attach.handle) {
    	vk_image_shut(ctx, &swap->image_attach);
		vk_image_view_shut(ctx, swap);
	}

	vk_image_view_init(ctx, swap);
    vk_image_init(ctx, VK_IMAGE_TYPE_2D, ctx->device.depth_format,
                  VK_IMAGE_TILING_OPTIMAL,
                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
                  &swap->image_attach);
	vk_image_view_from_image(ctx, &swap->image_attach,
                         VK_FORMAT_D32_SFLOAT,
                         VK_IMAGE_ASPECT_DEPTH_BIT,
                         &swap->image_attach.image_view);

    ar_DEBUG("Vulkan Swapchain Created");
}

/* ========================================================================== */
/* ========================================================================== */

void vk_swapchain_init(vulkan_context_t *ctx, vulkan_swapchain_t *swap) {
	vk_internal_swapchain(ctx, swap, VK_NULL_HANDLE);
}

b8 vk_swapchain_reinit(vulkan_context_t *ctx, vulkan_swapchain_t *swap) {
	VkSwapchainKHR old = swap->handle;
	vk_internal_swapchain(ctx, swap, old);

	vkDestroySwapchainKHR(ctx->device.logic_dev, old, ctx->alloc);

	return true;
}

b8 vk_swapchain_acquire_next_image(vulkan_context_t *ctx,
                                   VkSemaphore avail_sema, uint32_t *image_idx,
                                   vulkan_swapchain_t *swap) {
    VkResult result =
        vkAcquireNextImageKHR(ctx->device.logic_dev, swap->handle, UINT64_MAX,
                              avail_sema, VK_NULL_HANDLE, image_idx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		ar_WARNING("Swapchain out of date. Recreating...");
        vk_swapchain_reinit(ctx, swap);
        return false;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        ar_FATAL("Failed to acquire swapchain image");
        return false;
    }
    return true;
}

b8 vk_swapchain_present(vulkan_context_t *ctx, VkQueue graphics_queue,
                        VkQueue present_queue, VkSemaphore complete_semaphore,
                        uint32_t img_idx, vulkan_swapchain_t *swap) {
    (void)graphics_queue;
    VkPresentInfoKHR present_info   = {};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &complete_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swap->handle;
    present_info.pImageIndices      = &img_idx;

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        ar_WARNING("Swapchain is out of date. Recreating...");
		vk_swapchain_shut(ctx, swap);
		vk_swapchain_init(ctx, swap);
    }

	if (result == VK_SUBOPTIMAL_KHR) {
		ar_WARNING("Swapchain is Suboptimal. Recreating...");
		vk_swapchain_reinit(ctx, swap);
	}

    if (result != VK_SUCCESS) {
        return false;
	}

	ctx->current_frame = (ctx->current_frame + 1) % swap->max_frame_in_flight;

    return true;
}

void vk_image_view_init(vulkan_context_t *ctx, vulkan_swapchain_t *swap) {
    ctx->current_frame = 0;

    VK_CHECK(vkGetSwapchainImagesKHR(ctx->device.logic_dev,
                                     ctx->swapchain.handle, &swap->image_count,
                                     0));
	if (swap->handle) {
        memory_free(swap->image_view, sizeof(VkImageView) * swap->image_count,
                    MEMTAG_RENDERER);
        memory_free(swap->image_view, sizeof(VkImage) * swap->image_count,
                    MEMTAG_RENDERER);
    }

    swap->image =
        memory_alloc(sizeof(VkImage) * swap->image_count, MEMTAG_RENDERER);
    swap->image_view =
        memory_alloc(sizeof(VkImageView) * swap->image_count, MEMTAG_RENDERER);

    VK_CHECK(vkGetSwapchainImagesKHR(ctx->device.logic_dev,
                                     ctx->swapchain.handle, &swap->image_count,
                                     swap->image));


    for (uint32_t i = 0; i < swap->image_count; ++i) {
        VkImageViewCreateInfo img_view_info = {};
        img_view_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        img_view_info.image    = swap->image[i];
        img_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        img_view_info.format   = swap->image_format.format;
        img_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        img_view_info.subresourceRange.levelCount = 1;
        img_view_info.subresourceRange.layerCount = 1;
        img_view_info.subresourceRange.baseArrayLayer = 0;
        img_view_info.subresourceRange.baseMipLevel   = 0;

        VK_CHECK(vkCreateImageView(ctx->device.logic_dev, &img_view_info,
                                   ctx->alloc, &swap->image_view[i]));
		ar_TRACE("Created image view [%u]: %p", i, (void*)swap->image_view[i]);
    }
}

void vk_image_init(vulkan_context_t *ctx, VkImageType img_type,
                   VkFormat formats, VkImageTiling tiling,
                   VkImageUsageFlags usage, VkMemoryPropertyFlags mem_prop,
                   VkImageAspectFlags aspects, vulkan_image_t *image) {
	(void)img_type;
	(void)aspects;

	image->width = ctx->framebuffer_w;
	image->height = ctx->framebuffer_h;

	VkImageCreateInfo img_info = {};
	img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	img_info.imageType = VK_IMAGE_TYPE_2D;
	img_info.extent.width = ctx->framebuffer_w;
	img_info.extent.height = ctx->framebuffer_h;
	img_info.extent.depth = 1;
	img_info.mipLevels = 1;
	img_info.arrayLayers = 1;
	img_info.tiling = tiling;
	img_info.format = formats;
	img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	img_info.usage = usage;
	img_info.samples = VK_SAMPLE_COUNT_1_BIT;
	img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(ctx->device.logic_dev, &img_info, ctx->alloc,
                           &image->handle));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(ctx->device.logic_dev, image->handle, &mem_req);
	int32_t mem_type = ctx->find_mem_idx(mem_req.memoryTypeBits, mem_prop);
	if (mem_type == -1)
		ar_ERROR("Require memory type not found. Image not valid");

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = (uint32_t)mem_type;

    VK_CHECK(vkAllocateMemory(ctx->device.logic_dev, &alloc_info, ctx->alloc,
                              &image->memory));
    VK_CHECK(vkBindImageMemory(ctx->device.logic_dev, image->handle,
                               image->memory, 0));
}

void vk_image_view_from_image(vulkan_context_t *ctx, vulkan_image_t *image,
                              VkFormat format, VkImageAspectFlags aspect_mask,
                              VkImageView *out_view) {
    VkImageViewCreateInfo view_info = {0};
    view_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image                 = image->handle;
    view_info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format                = format;
    view_info.subresourceRange.aspectMask     = aspect_mask;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;

    VK_CHECK(vkCreateImageView(ctx->device.logic_dev, &view_info, ctx->alloc,
                               out_view));
}

void vk_swapchain_shut(vulkan_context_t *ctx, vulkan_swapchain_t *swap) {
	vk_image_view_shut(ctx, swap);
	vk_image_shut(ctx, &swap->image_attach);

    for (uint32_t i = 0; i < swap->image_count; ++i) {
        vkDestroyImageView(ctx->device.logic_dev, swap->image_view[i],
                           ctx->alloc);
    }
    vkDestroyImageView(ctx->device.logic_dev, swap->image_attach.image_view,
                       ctx->alloc);
    vkDestroySwapchainKHR(ctx->device.logic_dev, swap->handle, ctx->alloc);
}

void vk_image_view_shut(vulkan_context_t *ctx, vulkan_swapchain_t *swap) {
    if (swap->image_view) {
        for (uint32_t i = 0; i < swap->image_count; ++i) {
            if (swap->image_view[i]) {
                vkDestroyImageView(ctx->device.logic_dev, swap->image_view[i],
                                   ctx->alloc);
                swap->image_view[i] = VK_NULL_HANDLE;
            }
        }

        memory_free(swap->image_view, sizeof(VkImageView) * swap->image_count,
                    MEMTAG_RENDERER);
        swap->image_view = 0;
    }

    if (swap->image) {
        memory_free(swap->image, sizeof(VkImage) * swap->image_count,
                    MEMTAG_RENDERER);
        swap->image = 0;
    }

    swap->image_count = 0;
}

void vk_image_shut(vulkan_context_t *ctx, vulkan_image_t *image) {
	if (image->image_view) {
		vkDestroyImageView(ctx->device.logic_dev, image->image_view, ctx->alloc);
		image->image_view = 0;
	}

	if (image->memory) {
		vkFreeMemory(ctx->device.logic_dev, image->memory, ctx->alloc);
		image->memory = 0;
	}

	if (image->handle) {
		vkDestroyImage(ctx->device.logic_dev, image->handle, ctx->alloc);
		image->handle = 0;
	}
}
