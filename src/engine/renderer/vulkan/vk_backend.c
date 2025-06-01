#include "engine/renderer/vulkan/vk_backend.h"

#include "engine/core/application.h"

#include "engine/define.h"
#include "engine/core/logger.h"
#include "engine/math/math_type.h"

#include "engine/renderer/renderer_type.h"
#include "engine/renderer/vulkan/vk_type.h"
#include "engine/renderer/vulkan/vk_debug.h"
#include "engine/renderer/vulkan/vk_utils.h"
#include "engine/renderer/vulkan/vk_platform.h"
#include "engine/renderer/vulkan/vk_device.h"
#include "engine/renderer/vulkan/vk_swapchain.h"
#include "engine/renderer/vulkan/vk_renderpass.h"
#include "engine/renderer/vulkan/vk_combuffer.h"
#include "engine/renderer/vulkan/vk_result.h"
#include "engine/renderer/vulkan/vk_buffer.h"
#include "engine/renderer/vulkan/vk_image.h"

#include "engine/renderer/vulkan/shaders/vk_material_shader.h"
#include "engine/renderer/vulkan/shaders/vk_ui_shader.h"

#include "engine/resources/resc_type.h"
#include "engine/systems/material_sys.h"

#include <vulkan/vulkan_core.h>

// static vulkan context related
static vulkan_context_t context;
static uint32_t cache_framebuffer_width = 0;
static uint32_t cache_framebuffer_height = 0;

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
void upload_data(vulkan_context_t *ctx, VkCommandPool pool, VkFence fence,
                 VkQueue queue, vulkan_buffer_t *buffer, uint64_t offset,
                 uint64_t size, const void *data) {
    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vulkan_buffer_t staging;
    vk_buffer_init(ctx, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true,
                   &staging);

    vk_buffer_load_data(ctx, &staging, 0, size, 0, data);
    vk_buffer_copy(ctx, pool, fence, queue, staging.handle, 0, buffer->handle,
                   offset, size);
    vk_buffer_shut(ctx, &staging);
}

void free_data(vulkan_buffer_t *buffer, uint64_t offset, uint64_t size) {
	(void)buffer;
	(void)offset;
	(void)size;
	// TODO: Free in this buffer
	// TODO: using free-list
}

int32_t find_mem_idx(uint32_t type_filter, uint32_t prop_flag) {
	VkPhysicalDeviceMemoryProperties mem_prop;
	vkGetPhysicalDeviceMemoryProperties(context.device.phys_dev, &mem_prop);

	for (uint32_t i = 0; i < mem_prop.memoryTypeCount; ++i) {
		if (type_filter & (1 << i) &&
		   (mem_prop.memoryTypes[i].propertyFlags & prop_flag) == prop_flag)
			return (int32_t)i;
    }

	ar_WARNING("Unable to find suitable memory type");
	return -1;
}

b8 buffer_init(vulkan_context_t *ctx) {
    VkMemoryPropertyFlagBits mem_property_flag =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const uint64_t vertex_buffer_size = sizeof(vertex_3d) * 1024 * 1024;
    if (!vk_buffer_init(ctx, vertex_buffer_size,
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        mem_property_flag, true, &ctx->obj_vert_buffer)) {
        ar_ERROR("Error creating vertex buffer.");
        return false;
    }
    ctx->geo_vert_offset = 0;

    const uint64_t index_buffer_size = sizeof(uint32_t) * 1024 * 1024;
    if (!vk_buffer_init(ctx, index_buffer_size,
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        mem_property_flag, true, &ctx->obj_idx_buffer)) {
        ar_ERROR("Error creating index buffer");
        return false;
    }
    ctx->geo_idx_offset = 0;

    return true;
}

void command_buffer_init(render_backend_t *be) {
	(void)be;

    if (!context.graphic_comm_buffer) {
        context.graphic_comm_buffer =
            dyn_array_reserved(vulkan_commandbuffer_t,
                               context.swapchain.image_count);

        for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
            memory_zero(&context.graphic_comm_buffer[i],
                        sizeof(vulkan_commandbuffer_t));
        }
    }

    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        if (context.graphic_comm_buffer[i].handle) {
            vk_combuff_shut(&context, context.device.graphics_command_pool,
                            &context.graphic_comm_buffer[i]);
        }

        memory_zero(&context.graphic_comm_buffer[i],
                    sizeof(vulkan_commandbuffer_t));
        vk_combuff_init(&context, context.device.graphics_command_pool,
                        &context.graphic_comm_buffer[i]);
    }

    ar_DEBUG("Vulkan Commandbuffer Created");
}

void regen_framebuffer(void) {
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        /* World Pass */
        VkImageView world_attach[2] =
            {context.swapchain.image_view[i],
             context.swapchain.image_attach.image_view};
        VkFramebufferCreateInfo framebuffer_cr_info = {};
        framebuffer_cr_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_cr_info.renderPass      = context.main_render.handle;
        framebuffer_cr_info.attachmentCount = 2;
        framebuffer_cr_info.pAttachments    = world_attach;
        //framebuffer_cr_info.width           = context.framebuffer_w;
        //framebuffer_cr_info.height          = context.framebuffer_h;
        framebuffer_cr_info.width           = context.swapchain.extents.width;
        framebuffer_cr_info.height          = context.swapchain.extents.height;
        framebuffer_cr_info.layers          = 1;

        VK_CHECK(vkCreateFramebuffer(context.device.logic_dev,
                                     &framebuffer_cr_info, context.alloc,
                                     &context.world_framebuffer[i]));

        /* UI Pass */
        VkImageView ui_attach[1]           = {context.swapchain.image_view[i]};
        VkFramebufferCreateInfo sc_fb_info = {};
        sc_fb_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        sc_fb_info.renderPass      = context.ui_render.handle;
        sc_fb_info.attachmentCount = 1;
        sc_fb_info.pAttachments    = ui_attach;
        //sc_fb_info.width           = context.framebuffer_w;
        //sc_fb_info.height          = context.framebuffer_h;
        sc_fb_info.width           = context.swapchain.extents.width;
        sc_fb_info.height          = context.swapchain.extents.height;
        sc_fb_info.layers          = 1;

        VK_CHECK(vkCreateFramebuffer(context.device.logic_dev, &sc_fb_info,
                                     context.alloc,
                                     &context.swapchain.framebuffers[i]));
    }
}

b8 recreate_swapchain(render_backend_t *be) {
	if (context.recreate_swap) {
		ar_DEBUG("recreate_swapchain already creating");
		return false;
	}

	if (context.framebuffer_w == 0 || context.framebuffer_h == 0) {
		ar_DEBUG("recreate_swapchain called window < 1 in dimension. Booting...");
		return false;
	}
	
	context.recreate_swap = true;
	vkDeviceWaitIdle(context.device.logic_dev);

	vk_swapchain_reinit(&context, &context.swapchain);
	
	context.framebuffer_w = cache_framebuffer_width;
	context.framebuffer_h = cache_framebuffer_height;
	context.main_render.render_area.z = context.framebuffer_w;
	context.main_render.render_area.w = context.framebuffer_h;
	cache_framebuffer_height = 0;
	cache_framebuffer_width = 0;
	context.framebuffer_last_gen = context.framebuffer_size_gen;

    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        vk_combuff_shut(&context, context.device.graphics_command_pool,
                        &context.graphic_comm_buffer[i]);
    }
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        vkDestroyFramebuffer(context.device.logic_dev,
                             context.world_framebuffer[i], context.alloc);
        vkDestroyFramebuffer(context.device.logic_dev,
                             context.swapchain.framebuffers[i], context.alloc);
    }

    /* World Renderpass */
    context.main_render.render_area.x = 0;
    context.main_render.render_area.y = 0;
    context.main_render.render_area.z = context.framebuffer_w;
    context.main_render.render_area.w = context.framebuffer_h;

    /* UI Renderpass */
    context.ui_render.render_area.x = 0;
    context.ui_render.render_area.y = 0;
    context.ui_render.render_area.z = context.framebuffer_w;
    context.ui_render.render_area.w = context.framebuffer_h;

	regen_framebuffer();
	command_buffer_init(be);
	context.recreate_swap = false;

	return true;
}

void vk_cmd_image_barrier_between_passes(VkCommandBuffer cmd, VkImage image) {
    VkImageMemoryBarrier barrier = {};
    barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    // Source access mask: writing color attachment
    barrier.srcAccessMask = 0;
    // Destination access mask: reading/writing color attachment in next pass
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // src stage
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dst stage
        0, 0, NULL, 0, NULL, 1, &barrier);
}

/* ========================================================================== */
/* ========================================================================== */

b8 vk_backend_init(render_backend_t *backend, const char *name) {
	context.find_mem_idx = find_mem_idx;
	context.alloc = 0;

	application_get_framebuffer_size(&cache_framebuffer_width,
									 &cache_framebuffer_height);
	context.framebuffer_w =
		(cache_framebuffer_width != 0) ? cache_framebuffer_width : 1280;
	context.framebuffer_h =
		(cache_framebuffer_height != 0) ? cache_framebuffer_height : 720;
	cache_framebuffer_width = 0;
	cache_framebuffer_height = 0;


	/* =========================== Setup Vulkan ============================= */
	VkApplicationInfo app_info 	= {};
	app_info.sType 				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.apiVersion 		= VK_API_VERSION_1_0;
	app_info.pApplicationName 	= name;
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName 		= "My Engine";
	app_info.engineVersion 		= VK_MAKE_VERSION(1, 0, 0);
	
	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	/* ======================= Extensions Vulkan ============================ */
	uint32_t ext_count;
	const char **extension = vk_req_get_extensions(&ext_count);

	create_info.enabledExtensionCount = ext_count;
	create_info.ppEnabledExtensionNames = extension;

	/* ==================== Validation Layer Vulkan ========================= */
	uint32_t layer_count;
	const char **validation = vk_get_validation_layers(&layer_count);

	create_info.enabledLayerCount = layer_count;
	create_info.ppEnabledLayerNames = validation;

	VK_CHECK(vkCreateInstance(&create_info, context.alloc, &context.instance));
	ar_DEBUG("Vulkan Instance Created");

	/* ======================== Vulkan Debugger ============================= */
#if defined (_DEBUG)
	vk_debug_init(&context);
	ar_DEBUG("Vulkan Debug Messenger Created");
#endif

	/* ========================= Vulkan Surface ============================= */
	ar_DEBUG("Vulkan Surface Created");
	if (!platform_create_vulkan_surface(&context)) {
		ar_ERROR("Failed to create platform surface");
		return false;
	}

	/* ========================= Vulkan Device ============================== */
	ar_DEBUG("Vulkan Device Created");
	if (!vk_device_init(&context)) {
		ar_ERROR("Failed to create vulkan device");
		return false;
	}

	/* ======================== Vulkan Swapchain ============================= */
	vk_swapchain_init(&context, &context.swapchain);

	/* ======================= Vulkan Renderpass ============================= */
    // World Render Layer
    vk_renderpass_init(&context,
                       (vec4){.x = 0.0f,
                              .y = 0.0f,
                              .z = context.framebuffer_w,
                              .w = context.framebuffer_h},
                       (vec4){.x = 0.3f, 0.2f, 0.5f, 1.0f}, 1.0f, 0,
                       CLEAR_COLOR_BUFFER | CLEAR_DEPTH_BUFFER |
					   CLEAR_STENCIL_BUFFER,
                       false, true, &context.main_render);

    // UI Render Layer
    vk_renderpass_init(&context,
                       (vec4){.x = 0.0f,
                              .y = 0.0f,
                              .z = context.framebuffer_w,
                              .w = context.framebuffer_h},
                       (vec4){.x = 0.0f, 0.0f, 0.0f, 0.0f}, 1.0f, 0,
                       CLEAR_NON_FLAG, true, false, &context.ui_render);

    /* ======================= Vulkan Framebuffer ============================ */
	regen_framebuffer();
    command_buffer_init(backend);
	/* ======================== Vulkan Semaphore ============================= */
    context.avail_semaphore =
        dyn_array_reserved(VkSemaphore, context.swapchain.max_frame_in_flight);
    context.complete_semaphore =
        dyn_array_reserved(VkSemaphore, context.swapchain.max_frame_in_flight);

	for (uint8_t i = 0; i < context.swapchain.max_frame_in_flight; ++i) {
		VkSemaphoreCreateInfo sm_info = {};
		sm_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vkCreateSemaphore(context.device.logic_dev, &sm_info, context.alloc,
                          &context.avail_semaphore[i]);
        vkCreateSemaphore(context.device.logic_dev, &sm_info, context.alloc,
                          &context.complete_semaphore[i]);

        VkFenceCreateInfo fence_info = {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(context.device.logic_dev, &fence_info,
                               context.alloc, &context.in_flight_fence[i]));
    }

    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        context.image_in_flight[i] = 0;
    }

    /* Builtin Shaders */
    if (!vk_material_shader_init(&context, &context.material_shader)) {
		ar_ERROR("Error loading Builtin Material Shader");
		return false;
	}

    if (!vk_ui_shader_init(&context, &context.ui_shader)) {
        ar_ERROR("Error loading Builtin UI Shader");
        return false;
    }

	buffer_init(&context);

	/* Mark all geometries as INVALID_ID */
	for (uint32_t i = 0; i < VULKAN_GEOMETRY_MAX_COUNT; ++i) {
		context.geometries[i].id = INVALID_ID;
	}

    ar_DEBUG("Vulkan API Granted Access Successfully");

	/* this for clean up all the vulkan extensions */
	dyn_array_destroy(extension);
	dyn_array_destroy(validation);

    ar_INFO("Renderer System Initialized");
	return true;
}

void vk_backend_shut(render_backend_t *backend) {
	(void)backend;

	vkDeviceWaitIdle(context.device.logic_dev);

	ar_DEBUG("Kill Vulkan Buffer");
	vk_buffer_shut(&context, &context.obj_idx_buffer);
	vk_buffer_shut(&context, &context.obj_vert_buffer);

	ar_DEBUG("Kill Vulkan Object Shaders");
    vk_ui_shader_shut(&context, &context.ui_shader);
	vk_material_shader_shut(&context, &context.material_shader);

    for (uint8_t i = 0; i < context.swapchain.max_frame_in_flight; ++i) {
        if (context.avail_semaphore[i]) {
            vkDestroySemaphore(context.device.logic_dev,
                               context.avail_semaphore[i], context.alloc);
            context.avail_semaphore[i] = 0;
        }

        if (context.complete_semaphore[i]) {
            vkDestroySemaphore(context.device.logic_dev,
                               context.complete_semaphore[i], context.alloc);
            context.complete_semaphore[i] = 0;
        }
        vkDestroyFence(context.device.logic_dev, context.in_flight_fence[i],
                       context.alloc);
    }
    dyn_array_destroy(context.avail_semaphore);
	dyn_array_destroy(context.complete_semaphore);
	context.avail_semaphore = 0;
	context.complete_semaphore = 0;

	ar_DEBUG("Kill Vulkan Commandbuffer");
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        if (context.graphic_comm_buffer[i].handle) {
            vk_combuff_shut(&context, context.device.graphics_command_pool,
                            &context.graphic_comm_buffer[i]);
            context.graphic_comm_buffer[i].handle = 0;
        }
    }
    dyn_array_destroy(context.graphic_comm_buffer);
	context.graphic_comm_buffer = 0;

    ar_DEBUG("Kill Vulkan Framebuffer");
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        vkDestroyFramebuffer(context.device.logic_dev,
                             context.world_framebuffer[i], context.alloc);
        vkDestroyFramebuffer(context.device.logic_dev,
                             context.swapchain.framebuffers[i], context.alloc);
    }

    ar_DEBUG("Kill Vulkan Renderpass");
    vk_renderpass_shut(&context, &context.ui_render);
	vk_renderpass_shut(&context, &context.main_render);

	ar_DEBUG("Kill Vulkan Swapchain");
	vk_swapchain_shut(&context, &context.swapchain);

	ar_DEBUG("Kill Vulkan Device");
	vk_device_shut(&context);

	ar_DEBUG("Kill Vulkan Surface");
	if (context.surface) {
		vkDestroySurfaceKHR(context.instance, context.surface, context.alloc);
		context.surface = 0;
	}

#if defined (_DEBUG)
	ar_DEBUG("Kill Vulkan Debugger");
	vk_debug_shut(&context);
#endif

	ar_DEBUG("Kill Vulkan Instance");
	vkDestroyInstance(context.instance, context.alloc);
}

void vk_backend_resize(render_backend_t *backend, uint32_t width,
                       uint32_t height) {
  (void)backend;

  cache_framebuffer_width = (width > 0) ? width : 1;
  cache_framebuffer_height = (height > 0) ? height : 1;
  context.framebuffer_size_gen++;

  ar_DEBUG("Vulkan Render resize: w/h/gen= %i/%i/%i", width, height,
          context.framebuffer_size_gen);
}

b8 vk_backend_begin_frame(render_backend_t *backend, float delta_time) {
	(void)backend;
	context.frame_delta = delta_time;
	
    vkQueueWaitIdle(context.device.graphics_queue);
	if (context.recreate_swap) {
		VkResult result = vkDeviceWaitIdle(context.device.logic_dev);

		if (!vk_result_is_success(result)) {
			return false;
		}

		if (!recreate_swapchain(backend)) {
			return false;
		}

		context.recreate_swap = false;
	}

	if (context.framebuffer_size_gen != context.framebuffer_last_gen) {
		VkResult result = vkDeviceWaitIdle(context.device.logic_dev);

		if (!vk_result_is_success(result)) {
            ar_ERROR("vkDeviceWaitIdle (2) failed: '%s'",
                     vk_result_string(result, true));
            return false;
        }

        if (!recreate_swapchain(backend)) {
            return false;
        }

        ar_INFO("Resized Booting");
		return false;
	}

    /* Wait execution frame_in_flight */
    //vkQueueWaitIdle(context.device.graphics_queue);
    VkResult result =
        vkWaitForFences(context.device.logic_dev, 1,
                        &context.in_flight_fence[context.current_frame], true,
                        UINT64_MAX);
    if (!vk_result_is_success(result)) {
        ar_ERROR("In-flight fence wait failed. Error: %s",
                 vk_result_string(result, true));
        return false;
    }

    /* Get Next Image */
    if (!vk_swapchain_acquire_next_image(&context,
                                         context.avail_semaphore
                                             [context.current_frame],
                                         &context.image_idx,
                                         &context.swapchain)) {
        return false;
    }

	if (context.image_idx >= context.swapchain.image_count) {
        ar_ERROR("Invalid image index: %u", context.image_idx);
        return false;
    }

	/* Begin Record Command */
    vulkan_commandbuffer_t *combuff =
        &context.graphic_comm_buffer[context.image_idx];
	vk_combuff_reset(combuff);
    vk_combuff_begin(combuff);

    /* Viewport & Scissor */
	/* This is was Dynamic Viewport */
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)context.framebuffer_w;
	viewport.height = (float)context.framebuffer_h;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset.x = scissor.offset.y = 0;
	scissor.extent.width = context.framebuffer_w;
	scissor.extent.height = context.framebuffer_h;
	vkCmdSetViewport(combuff->handle, 0, 1, &viewport);
	vkCmdSetScissor(combuff->handle, 0, 1, &scissor);

	context.main_render.render_area.z = context.framebuffer_w;
	context.main_render.render_area.w = context.framebuffer_h;
	context.ui_render.render_area.z = context.framebuffer_w;
	context.ui_render.render_area.w = context.framebuffer_h;

    return true;
}

void vk_backend_update_world(mat4 projection, mat4 view, vec3 view_pos,
                              vec4 ambient_color, int32_t mode) {
	(void)view_pos;
	(void)ambient_color;
	(void)mode;
	vk_material_shader_use(&context, &context.material_shader);
	context.material_shader.global_ubo.projection = projection;
	context.material_shader.global_ubo.view = view;

	// TODO: other UBO properties

    vk_material_shader_update_global_state(&context, &context.material_shader,
                                      context.frame_delta);
}

void vk_backend_update_ui(mat4 projection, mat4 view, int32_t mode) {
    (void)mode;
    vk_ui_shader_use(&context, &context.ui_shader);
    context.ui_shader.global_ubo.projection = projection;
    context.ui_shader.global_ubo.view = view;

    // TODO: other UBO properties

    vk_ui_shader_update_global(&context, &context.ui_shader, context.frame_delta);
}

b8 vk_backend_end_frame(render_backend_t *backend, float delta_time) {
	(void)backend;
	context.frame_delta = delta_time;

    vulkan_commandbuffer_t *combuff =
        &context.graphic_comm_buffer[context.image_idx];
    //vk_renderpass_end(combuff);
    vk_combuff_end(combuff);

    if (context.image_in_flight[context.image_idx] != 0) {
        VkResult result =
            vkWaitForFences(context.device.logic_dev, 1,
                            &context.image_in_flight[context.image_idx], true,
                            UINT64_MAX);
        if (!vk_result_is_success(result)) {
            ar_FATAL("vkWaitForFences error: %s",
                     vk_result_string(result, true));
        }
    }

    context.image_in_flight[context.image_idx] =
        context.in_flight_fence[context.current_frame];
    VK_CHECK(vkResetFences(context.device.logic_dev, 1,
                           &context.in_flight_fence[context.current_frame]));

    /* Begin Submit */
    VkSubmitInfo submit_info         = {};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &combuff->handle;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores =
        &context.complete_semaphore[context.current_frame];
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores =
        &context.avail_semaphore[context.current_frame];

    VkPipelineStageFlags flags[1] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = flags;

    VkResult result =
        vkQueueSubmit(context.device.graphics_queue, 1, &submit_info,
                      context.in_flight_fence[context.current_frame]);

	if (result != VK_SUCCESS) {
		ar_ERROR("vkQueueSubmit error");
		return false;
	}

	vk_combuff_update(combuff);

    vk_swapchain_present(&context, context.device.graphics_queue,
                         context.device.present_queue,
                         context.complete_semaphore[context.current_frame],
                         context.image_idx, &context.swapchain);

    return true;
}

b8 vk_backend_begin_renderpass(render_backend_t *be, uint8_t renderpass_id) {
    (void)be;
    vulkan_renderpass_t    *renderpass  = 0;
    VkFramebuffer           framebuffer = 0;
    vulkan_commandbuffer_t *combuff =
        &context.graphic_comm_buffer[context.image_idx];

    // Choose renderpass ID
    switch (renderpass_id) {
    case RENDER_LAYER_WORLD:
        renderpass  = &context.main_render;
        framebuffer = context.world_framebuffer[context.image_idx];
        break;

    case RENDER_LAYER_UI:
        renderpass  = &context.ui_render;
        framebuffer = context.swapchain.framebuffers[context.image_idx];
        break;

    default:
        ar_ERROR("vk_backend_begin_renderpass - called on unrecognized ID: "
                 "%#02x", renderpass_id);
        return false;
    }

    vk_renderpass_begin(combuff, renderpass, framebuffer);

    switch (renderpass_id) {
    case RENDER_LAYER_WORLD:
        vk_material_shader_use(&context, &context.material_shader);
        break;

    case RENDER_LAYER_UI:
        vk_ui_shader_use(&context, &context.ui_shader);
        break;
    }
    
    return true;
}

b8 vk_backend_end_renderpass(render_backend_t *be, uint8_t renderpass_id) {
    (void)be;
    vulkan_renderpass_t *renderpass = 0;
    vulkan_commandbuffer_t *combuff =
        &context.graphic_comm_buffer[context.image_idx];

    // Choose renderpass ID
    switch (renderpass_id) {
    case RENDER_LAYER_WORLD:
        renderpass = &context.main_render;
        break;

    case RENDER_LAYER_UI:
        renderpass = &context.ui_render;
        break;

    default:
        ar_ERROR("vk_backend_end_renderpass - called on unrecognized ID: "
                 "%#02x", renderpass_id);
        return false;
    }

    vk_renderpass_end(combuff, renderpass);
    return true;
}

void vk_backend_tex_init(const uint8_t *pixel, texture_t *texture) {
	texture->gen = INVALID_ID;

	// TODO: use an allocator
    texture->internal_data =
        (vulkan_texture_data_t *)memory_alloc(sizeof(vulkan_texture_data_t),
                                              MEMTAG_TEXTURE);
    vulkan_texture_data_t *data =
        (vulkan_texture_data_t *)texture->internal_data;
	VkDeviceSize image_size = texture->width * texture->height * texture->channel_count;

	// NOTE: 8-bit per channel.
	VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

	/* Create staging buffer & load data */
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags mem_prop_flag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkan_buffer_t staging;
    vk_buffer_init(&context, image_size, usage, mem_prop_flag, true, &staging);
	vk_buffer_load_data(&context, &staging, 0, image_size, 0, pixel);

    vk_image_init(&context, VK_IMAGE_TYPE_2D, image_format,
                  VK_IMAGE_TILING_OPTIMAL, texture->width, texture->height,
                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                  mem_prop_flag, VK_IMAGE_ASPECT_COLOR_BIT, &data->image);

    vulkan_commandbuffer_t temp_buff;
	VkCommandPool pool = context.device.graphics_command_pool;
	VkQueue queue = context.device.graphics_queue;
	vk_combuff_single_use_init(&context, pool, &temp_buff);

	/* Transition current layout to optimal for receive data */
    vk_image_transition_layout(&context, &temp_buff, &data->image,
                               &image_format, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	/* Copy data from framebuffer */
	vk_image_copy_buffer(&context, &data->image, staging.handle, &temp_buff);

    /* Transition from optimal data receive to shader read-only optimal layout */
    vk_image_transition_layout(&context, &temp_buff, &data->image,
                               &image_format,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk_combuff_single_use_shut(&context, pool, &temp_buff, queue);

	vk_buffer_shut(&context, &staging);

    /* Create sampler for texture */
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;

	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	sampler_info.anisotropyEnable = VK_FALSE;

	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	sampler_info.mipLodBias = 0;
	sampler_info.minLod = 0;
	sampler_info.maxLod = 0;

    VkResult result = vkCreateSampler(context.device.logic_dev, &sampler_info,
                                      context.alloc, &data->sampler);
    if (!vk_result_is_success(VK_SUCCESS)) {
        ar_ERROR("Error creating texture sampler: %s",
                 vk_result_string(result, true));
        return;
    }

	texture->gen++;
}

void vk_backend_tex_shut(texture_t *texture) {
    vkDeviceWaitIdle(context.device.logic_dev);

    vulkan_texture_data_t *data =
        (vulkan_texture_data_t *)texture->internal_data;

    if (data) {
        vk_image_shut(&context, &data->image);
        memory_zero(&data->image, sizeof(vulkan_image_t));

        vkDestroySampler(context.device.logic_dev, data->sampler,
                         context.alloc);
        data->sampler = 0;
        memory_free(texture->internal_data, sizeof(vulkan_texture_data_t),
                    MEMTAG_TEXTURE);
    }

    memory_zero(texture, sizeof(texture_t));
}

b8 vk_backend_material_init(material_t *material) {
    if (material) {
        switch (material->type) {
        case MATERIAL_TYPE_WORLD:
            if (!vk_material_shader_acquire_rsc(&context,
                                                &context.material_shader,
                                                material)) {
                ar_ERROR("vK_renderer_material_init - Failed to acquire world "
                         "shader.");
                return false;
            }
            break;

        case MATERIAL_TYPE_UI:
            if (!vk_ui_shader_acquire_resc(&context, &context.ui_shader,
                                           material)) {
                ar_ERROR(
                    "vK_renderer_material_init - Failed to acquire UI shader.");
                return false;
            }
            break;
        default:
            ar_ERROR("vk_backend_material_init - unknown material type.");
            return false;
        }

        ar_TRACE("Renderer: Material created.");
        return true;
    }

    ar_ERROR("vulkan_renderer_create_material called with nullptr. Creation "
             "failed.");
    return false;
}

void vk_backend_material_shut(material_t *material) {
    if (material) {
        if (material->internal_id != INVALID_ID) {
            switch (material->type) {
            case MATERIAL_TYPE_WORLD:
                vk_material_shader_release_rsc(&context,
                                               &context.material_shader,
                                               material);
                break;

            case MATERIAL_TYPE_UI:
                vk_ui_shader_release_resc(&context, &context.ui_shader,
                                          material);
                break;
            }
        } else {
            ar_WARNING("vk_backend_material_shut called with "
                       "internal_id=INVALID_ID. Nothing was done");
        }
    } else {
        ar_WARNING(
            "vk_backend_material_shut called with nullptr. Nothing was done");
    }
}

b8 vk_backend_geometry_init(geometry_t *geometry, uint32_t vertex_size, uint32_t vertex_count,
                            const void *vertices, uint32_t idx_size, uint32_t idx_count,
                            const void *indices) {
    if (!vertex_count || !vertices) {
        ar_ERROR("vk_backend_geometry_init - require vertex data, none was "
                 "supplied. vertex_count=%d, vertices=%p",
                 vertex_count, vertices);
        return false;
    }

    // Check if this was re-upload.
    b8                 is_reupload = geometry->internal_id != INVALID_ID;
    vulkan_geo_data_t  old_data;
    vulkan_geo_data_t *internal_data = 0;

    if (is_reupload) {
        internal_data              = &context.geometries[geometry->internal_id];

        // take copy of old data
        old_data.idx_buffer_offset = internal_data->idx_buffer_offset;
        old_data.idx_count         = internal_data->idx_count;
        old_data.idx_element_size  = internal_data->idx_element_size;
        old_data.vertex_buffer_offset = internal_data->vertex_buffer_offset;
        old_data.vertex_count         = internal_data->vertex_count;
        old_data.vertex_element_size  = internal_data->vertex_element_size;
    } else {
        for (uint32_t i = 0; i < VULKAN_GEOMETRY_MAX_COUNT; ++i) {
            if (context.geometries[i].id == INVALID_ID) {
                geometry->internal_id    = i;
                context.geometries[i].id = i;
                internal_data            = &context.geometries[i];
                break;
            }
        }
    }

    if (!internal_data) {
        ar_FATAL("vk_backend_geometry_init - failed to find free index for "
                 "geometry upload. Adjust config to allow more");
        return false;
    }

    VkCommandPool pool                  = context.device.graphics_command_pool;
    VkQueue       queue                 = context.device.graphics_queue;

    /* Vertex Data */
    internal_data->vertex_buffer_offset = context.geo_vert_offset;
    internal_data->vertex_count         = vertex_count;
    internal_data->vertex_element_size  = sizeof(vertex_3d);
	uint32_t total_size = vertex_count * vertex_size;
    upload_data(&context, pool, 0, queue, &context.obj_vert_buffer,
                internal_data->vertex_buffer_offset, total_size,
                vertices);
    // TODO: using free list instead of this.
    context.geo_vert_offset += total_size;

    if (idx_count && indices) {
        internal_data->idx_buffer_offset = context.geo_idx_offset;
        internal_data->idx_count         = idx_count;
        internal_data->idx_element_size  = sizeof(uint32_t);
		total_size = idx_count * idx_size;
        upload_data(&context, pool, 0, queue, &context.obj_idx_buffer,
                    internal_data->idx_buffer_offset, total_size,
                    indices);
        // TODO: using free list intead of this.
        context.geo_idx_offset += total_size;
    }

    if (internal_data->gen == INVALID_ID) {
        internal_data->gen = 0;
    } else {
        internal_data->gen++;
    }

    if (is_reupload) {
		// Vertex Data 
        free_data(&context.obj_vert_buffer, old_data.vertex_buffer_offset,
                  old_data.vertex_element_size * old_data.vertex_count);

        if (old_data.idx_element_size > 0) {
			// Index Data
            free_data(&context.obj_idx_buffer, old_data.idx_buffer_offset,
                      old_data.idx_element_size * old_data.idx_count);
        }
    }

    return true;
}

void vk_backend_geometry_shut(geometry_t *geometry) {
    if (geometry && geometry->internal_id != INVALID_ID) {
        vkDeviceWaitIdle(context.device.logic_dev);
        vulkan_geo_data_t *internal_data =
            &context.geometries[geometry->internal_id];

        /* Free Vertex Data */
        free_data(&context.obj_vert_buffer, internal_data->vertex_buffer_offset,
                  internal_data->vertex_element_size * internal_data->vertex_count);

        /* Free Index Data */
        if (internal_data->idx_element_size > 0) {
            free_data(&context.obj_idx_buffer, internal_data->idx_buffer_offset,
                      internal_data->idx_element_size * internal_data->idx_count);
        }

        memory_zero(internal_data, sizeof(vulkan_geo_data_t));
        internal_data->id  = INVALID_ID;
        internal_data->gen = INVALID_ID;
    }
}

void vk_backend_geo_render(geo_render_data_t data) {
    if (data.geometry && data.geometry->internal_id == INVALID_ID) {
        return;
    }

    vulkan_geo_data_t *buffer_data =
        &context.geometries[data.geometry->internal_id];
    vulkan_commandbuffer_t *combuff =
        &context.graphic_comm_buffer[context.image_idx];

    material_t *m = 0;
    if (data.geometry->material) {
        m = data.geometry->material;
    } else {
        m = material_sys_get_default();
    }

    switch (m->type) {
        case MATERIAL_TYPE_WORLD:
        vk_material_shader_set_model(&context, &context.material_shader, data.model);
        vk_material_shader_apply(&context, &context.material_shader, m);
        break;

        case MATERIAL_TYPE_UI:
        vk_ui_shader_set_model(&context, &context.ui_shader, data.model);
        vk_ui_shader_apply_material(&context, &context.ui_shader, m);
        break;

        default:
        ar_ERROR("vk_backend_geo_render - unknown material type: %i", m->type);
        return;
    }

    VkDeviceSize offset[1] = {buffer_data->vertex_buffer_offset};
    vkCmdBindVertexBuffers(combuff->handle, 0, 1,
                           &context.obj_vert_buffer.handle,
                           (VkDeviceSize *)offset);

    // Draw
    if (buffer_data->idx_count > 0) {
        vkCmdBindIndexBuffer(combuff->handle, context.obj_idx_buffer.handle,
                             buffer_data->idx_buffer_offset,
                             VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(combuff->handle, buffer_data->idx_count, 1, 0, 0, 0);
    } else {
        vkCmdDraw(combuff->handle, buffer_data->vertex_count, 1, 0, 0);
    }
}
