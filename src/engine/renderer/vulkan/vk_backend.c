#include "engine/renderer/vulkan/vk_backend.h"

#include "engine/core/application.h"
#include "engine/core/logger.h"
#include "engine/math/math_type.h"

#include "engine/renderer/vulkan/vk_type.h"
#include "engine/renderer/vulkan/vk_debug.h"
#include "engine/renderer/vulkan/vk_utils.h"
#include "engine/renderer/vulkan/vk_platform.h"
#include "engine/renderer/vulkan/vk_device.h"
#include "engine/renderer/vulkan/vk_swapchain.h"
#include "engine/renderer/vulkan/vk_renderpass.h"
#include "engine/renderer/vulkan/vk_combuffer.h"
#include "engine/renderer/vulkan/vk_framebuffer.h"
#include "engine/renderer/vulkan/vk_fence.h"
#include "engine/renderer/vulkan/vk_result.h"
#include "engine/renderer/vulkan/vk_buffer.h"
#include "engine/renderer/vulkan/vk_image.h"

// Shaders
#include "engine/renderer/vulkan/shaders/vk_material_shader.h"

#include <vulkan/vulkan_core.h>

// static vulkan context related
static vulkan_context_t context;
static uint32_t cache_framebuffer_width = 0;
static uint32_t cache_framebuffer_height = 0;

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
void upload_data(vulkan_context_t *ctx, VkCommandPool pool, VkFence fence,
                 VkQueue queue, vulkan_buffer_t *buffer, uint64_t offset,
                 uint64_t size, void *data) {
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

void draw_triangle(vulkan_context_t *ctx, VkCommandPool *pool, VkQueue *queues) {
#define VERT_COUNT 4
	vertex_3d verts[VERT_COUNT];
	memory_zero(verts, sizeof(vertex_3d) * VERT_COUNT);

	const float f = 10.0f;

	// Top Left
	verts[0].position.x = -0.5f * f;
	verts[0].position.y = -0.5f * f;
	verts[0].texcoord.x = 0.0f;
	verts[0].texcoord.y = 0.0f;

	// Bottom Right
	verts[1].position.x = 0.5f * f;
	verts[1].position.y = 0.5f * f;
	verts[1].texcoord.x = 1.0f;
	verts[1].texcoord.y = 1.0f;

	// Bottom Left
	verts[2].position.x = -0.5f * f;
	verts[2].position.y = 0.5f * f;
	verts[2].texcoord.x = 0.0f;
	verts[2].texcoord.y = 1.0f;

	// Top Right
	verts[3].position.x = 0.5f * f;
	verts[3].position.y = -0.5f * f;
	verts[3].texcoord.x = 1.0f;
	
#define IDX_COUNT 6
	uint32_t indices[IDX_COUNT] = {0, 1, 2, 0, 3, 1};

    upload_data(ctx, *pool, 0, *queues, &ctx->obj_vert_buffer, 0,
                sizeof(vertex_3d) * VERT_COUNT, verts);

    upload_data(ctx, *pool, 0, *queues, &ctx->obj_idx_buffer, 0,
                sizeof(uint32_t) * IDX_COUNT, indices);

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

void regen_framebuffer(render_backend_t *be, vulkan_swapchain_t *swapchain,
                       vulkan_renderpass_t *renderpass) {
	(void)be;
	for (uint32_t i = 0; i < swapchain->image_count; ++i) {
		uint32_t attach_count = 2;
        VkImageView attach[]     = {swapchain->image_view[i],
                                    swapchain->image_attach.image_view};

        vk_framebuffer_init(&context, renderpass, swapchain->extents, attach,
                            attach_count, &context.swapchain.framebuffer[i]);
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

	//vk_image_view_shut(&context, &context.swapchain);
	//vk_image_shut(&context, &context.swapchain.image_attach);
	vk_swapchain_reinit(&context, &context.swapchain);
	
	context.framebuffer_w = cache_framebuffer_width;
	context.framebuffer_h = cache_framebuffer_height;
	context.main_render.extents.width = context.framebuffer_w;
	context.main_render.extents.height = context.framebuffer_h;
	cache_framebuffer_height = 0;
	cache_framebuffer_width = 0;
	context.framebuffer_last_gen = context.framebuffer_size_gen;

    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        vk_combuff_shut(&context, context.device.graphics_command_pool,
                        &context.graphic_comm_buffer[i]);
    }
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        vk_framebuffer_shut(&context, &context.swapchain.framebuffer[i]);
    }

    context.main_render.extents.width = 0;
	context.main_render.extents.height = 0;
	context.main_render.extents.width = context.framebuffer_w;
	context.main_render.extents.height = context.framebuffer_h;

	regen_framebuffer(be, &context.swapchain, &context.main_render);
	command_buffer_init(be);
	context.recreate_swap = false;

	return true;
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
	vk_renderpass_init(&context, &context.main_render);

	/* ======================= Vulkan Framebuffer ============================ */
    context.swapchain.framebuffer =
        dyn_array_reserved(vulkan_framebuffer_t, context.swapchain.image_count);
	regen_framebuffer(backend, &context.swapchain, &context.main_render);
	
    command_buffer_init(backend);
	/* ======================== Vulkan Semaphore ============================= */
    context.avail_semaphore =
        dyn_array_reserved(VkSemaphore, context.swapchain.max_frame_in_flight);
    context.complete_semaphore =
        dyn_array_reserved(VkSemaphore, context.swapchain.max_frame_in_flight);
    context.in_flight_fence =
        dyn_array_reserved(vulkan_fence_t, context.swapchain.max_frame_in_flight);

	for (uint8_t i = 0; i < context.swapchain.max_frame_in_flight; ++i) {
		VkSemaphoreCreateInfo sm_info = {};
		sm_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vkCreateSemaphore(context.device.logic_dev, &sm_info, context.alloc,
                          &context.avail_semaphore[i]);
        vkCreateSemaphore(context.device.logic_dev, &sm_info, context.alloc,
                          &context.complete_semaphore[i]);

        vk_fence_reset(&context, &context.in_flight_fence[i]);
		vk_fence_init(&context, true, &context.in_flight_fence[i]);
	}

    context.image_in_flight =
        dyn_array_reserved(vulkan_fence_t, context.swapchain.image_count);
    for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
        context.image_in_flight[i] = 0;
    }

    if (!vk_material_shader_init(&context, &context.obj_shader)) {
		ar_ERROR("Error loading Builtin shader");
		return false;
	}

	buffer_init(&context);

	ar_INFO("--Test Drawing Triangle");
    draw_triangle(&context, &context.device.graphics_command_pool,
                  &context.device.graphics_queue);

	uint32_t obj_id = 0;
	if (!vk_material_shader_acquire_rsc(&context, &context.obj_shader, &obj_id)) {
		ar_ERROR("Failed to acquire shader resources");
		return false;
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
	vk_material_shader_shut(&context, &context.obj_shader);

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
        vk_fence_shut(&context, &context.in_flight_fence[i]);
    }
    dyn_array_destroy(context.avail_semaphore);
	dyn_array_destroy(context.complete_semaphore);
	dyn_array_destroy(context.in_flight_fence);
	dyn_array_destroy(context.image_in_flight);
	context.avail_semaphore = 0;
	context.complete_semaphore = 0;
	context.in_flight_fence = 0;
	context.image_in_flight = 0;

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
		vk_framebuffer_shut(&context, &context.swapchain.framebuffer[i]);
	}

	ar_DEBUG("Kill Vulkan Renderpass");
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
    if (!vk_fence_wait(&context,
                       &context.in_flight_fence[context.current_frame],
                       UINT64_MAX)) {
        ar_WARNING("In-flight fence wait failed");
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
	scissor.extent.width = context.swapchain.extents.width;
	scissor.extent.height = context.swapchain.extents.height;
	vkCmdSetViewport(combuff->handle, 0, 1, &viewport);
	vkCmdSetScissor(combuff->handle, 0, 1, &scissor);

	context.main_render.extents.width = context.framebuffer_w;
	context.main_render.extents.height = context.framebuffer_h;

    vk_renderpass_begin(combuff, &context.main_render,
                        context.swapchain.framebuffer[context.image_idx]
                            .handle);

    return true;
}

void vk_backend_update_global(mat4 projection, mat4 view, vec3 view_pos,
                              vec4 ambient_color, int32_t mode) {
	(void)view_pos;
	(void)ambient_color;
	(void)mode;
	vk_material_shader_use(&context, &context.obj_shader);
	context.obj_shader.global_ubo.projection = projection;
	context.obj_shader.global_ubo.viewx = view;

	// TODO: other UBO properties

    vk_material_shader_update_global_state(&context, &context.obj_shader,
                                      context.frame_delta);
}

b8 vk_backend_end_frame(render_backend_t *backend, float delta_time) {
	(void)backend;
	context.frame_delta = delta_time;

    vulkan_commandbuffer_t *combuff =
        &context.graphic_comm_buffer[context.image_idx];
    vk_renderpass_end(combuff);
    vk_combuff_end(combuff);

    if (context.image_in_flight[context.image_idx] != VK_NULL_HANDLE) {
        vk_fence_wait(&context, context.image_in_flight[context.image_idx],
                      UINT64_MAX);
    }

    context.image_in_flight[context.image_idx] =
        &context.in_flight_fence[context.current_frame];
    vk_fence_reset(&context, &context.in_flight_fence[context.current_frame]);

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
                      context.in_flight_fence[context.current_frame].handle);

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

void vk_backend_update_obj(geo_render_data_t data) {
	vk_material_shader_update_obj(&context, &context.obj_shader, data);

	// TODO: Temporary Code Test
	vk_material_shader_use(&context, &context.obj_shader);
	VkDeviceSize offset[1] = {0};

    vkCmdBindDescriptorSets(context.graphic_comm_buffer[context.image_idx]
                                .handle,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            context.obj_shader.pipeline.pipe_layout, 0, 1,
                            &context.obj_shader
                                 .global_desc_sets[context.image_idx],
                            0, NULL);
    // Bind vertex
    vkCmdBindVertexBuffers(context.graphic_comm_buffer[context.image_idx]
                               .handle,
                           0, 1, &context.obj_vert_buffer.handle,
                           (VkDeviceSize *)offset);
    // Bind indices
    vkCmdBindIndexBuffer(context.graphic_comm_buffer[context.image_idx].handle,
                         context.obj_idx_buffer.handle, 0,
                         VK_INDEX_TYPE_UINT32);

    // draw
    vkCmdDrawIndexed(context.graphic_comm_buffer[context.image_idx].handle,
                     IDX_COUNT, 1, 0, 0, 0);
}

void vk_backend_tex_init(const char *name, int32_t width,
                         int32_t height, int32_t channel_count,
                         const uint8_t *pixel, b8 has_transparent,
                         texture_t *texture) {
	(void)name;
	texture->width = (uint32_t)width;
	texture->height = (uint32_t)height;
	texture->channel_count = channel_count;
	texture->gen = INVALID_ID;

	// TODO: use an allocator
    texture->interal_data =
        (vulkan_texture_data_t *)memory_alloc(sizeof(vulkan_texture_data_t),
                                              MEMTAG_TEXTURE);
    vulkan_texture_data_t *data =
        (vulkan_texture_data_t *)texture->interal_data;
	VkDeviceSize image_size = (uint64_t)width * (uint64_t)height * (uint64_t)channel_count;

	// NOTE: assume 8-bit per channel.
	VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

	/* Create staging buffer & load data */
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags mem_prop_flag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkan_buffer_t staging;
    vk_buffer_init(&context, image_size, usage, mem_prop_flag, true, &staging);
	vk_buffer_load_data(&context, &staging, 0, image_size, 0, pixel);

    vk_image_init(&context, VK_IMAGE_TYPE_2D, image_format,
                  VK_IMAGE_TILING_OPTIMAL, (uint32_t)width, (uint32_t)height,
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

	texture->has_transparent = has_transparent;
	texture->gen++;
}

void vk_backend_tex_shut(texture_t *texture) {
    vkDeviceWaitIdle(context.device.logic_dev);

    vulkan_texture_data_t *data =
        (vulkan_texture_data_t *)texture->interal_data;

    if (data) {
        vk_image_shut(&context, &data->image);
        memory_zero(&data->image, sizeof(vulkan_image_t));

        vkDestroySampler(context.device.logic_dev, data->sampler,
                         context.alloc);
        data->sampler = 0;
        memory_free(texture->interal_data, sizeof(vulkan_texture_data_t),
                    MEMTAG_TEXTURE);
    }

    memory_zero(texture, sizeof(texture_t));
}
