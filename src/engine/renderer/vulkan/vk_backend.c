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
#include "engine/renderer/vulkan/vk_pipeline.h"

#include "engine/systems/shader_sys.h"
#include "engine/systems/texture_sys.h"
#include "engine/systems/resource_sys.h"


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
                   false, &staging);

    vk_buffer_load_data(ctx, &staging, 0, size, 0, data);
    vk_buffer_copy(ctx, pool, fence, queue, staging.handle, 0, buffer->handle,
                   offset, size);
    vk_buffer_shut(ctx, &staging);
}

void free_data(vulkan_buffer_t *buffer, uint64_t offset, uint64_t size) {
	if (buffer) {
		vk_buffer_free(buffer, size, offset);
	}
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
                        mem_property_flag, true, true, &ctx->obj_vert_buffer)) {
        ar_ERROR("Error creating vertex buffer.");
        return false;
    }

    const uint64_t index_buffer_size = sizeof(uint32_t) * 1024 * 1024;
    if (!vk_buffer_init(ctx, index_buffer_size,
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        mem_property_flag, true, true, &ctx->obj_idx_buffer)) {
        ar_ERROR("Error creating index buffer");
        return false;
    }

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

b8 create_module(vulkan_shader_t *shader, vulkan_shader_stage_config_t config,
                 vulkan_shader_stage_t *shader_stage) {
	(void)shader;
    // Read the resource.
    resource_t binary_resource;
    if (!resource_sys_load(config.filename, RESC_TYPE_BINARY,
                           &binary_resource)) {
        ar_ERROR("Unable to read shader module: %s.", config.filename);
        return false;
    }

    memory_zero(&shader_stage->cr_info, sizeof(VkShaderModuleCreateInfo));
    shader_stage->cr_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // Use the resource's size and data directly.
    shader_stage->cr_info.codeSize = binary_resource.data_size;
    shader_stage->cr_info.pCode    = (uint32_t *)binary_resource.data;

    VK_CHECK(vkCreateShaderModule(context.device.logic_dev,
                                  &shader_stage->cr_info, context.alloc,
                                  &shader_stage->handle));

    // Release the resource.
    resource_sys_unload(&binary_resource);

    // Shader stage info
    memory_zero(&shader_stage->shader_stg_cr_info,
                sizeof(VkPipelineShaderStageCreateInfo));
    shader_stage->shader_stg_cr_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage->shader_stg_cr_info.stage  = config.stage;
    shader_stage->shader_stg_cr_info.module = shader_stage->handle;
    shader_stage->shader_stg_cr_info.pName  = "main";

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
    vk_buffer_init(&context, image_size, usage, mem_prop_flag, true, false, &staging);
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

const uint32_t DESC_SET_INDEX_GLOBAL = 0;
const uint32_t DESC_SET_INDEX_INSTANCE = 1;
const uint32_t BINDING_INDEX_UBO = 0;
const uint32_t BINDING_INDEX_SAMPLER = 1;

b8 vk_backend_shader_create(struct shader_t *shader, uint8_t renderpass_id,
                            uint8_t stage_count, const char **stage_filenames,
                            shader_stage_t *stages) {
    shader->internal_data =
        memory_alloc(sizeof(vulkan_shader_t), MEMTAG_RENDERER);
    vulkan_renderpass_t *renderpass =
        renderpass_id == 1 ? &context.main_render : &context.ui_render;

    VkShaderStageFlags vk_stages[VULKAN_SHADER_MAX_STAGES];
    for (uint8_t i = 0; i < stage_count; ++i) {
        switch (stages[i]) {
        case SHADER_STAGE_FRAGMENT:
            vk_stages[i] = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        case SHADER_STAGE_VERTEX:
            vk_stages[i] = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case SHADER_STAGE_GEOMETRY:
            ar_WARNING(
                "VK_SHADER_STAGE_GEOMETRY_BIT is set but not yet supported");
            vk_stages[i] = VK_SHADER_STAGE_GEOMETRY_BIT;
            break;
        case SHADER_STAGE_COMPUTE:
            ar_WARNING(
                "VK_SHADER_STAGE_COMPUTE_BIT is set but not yet supported");
            vk_stages[i] = VK_SHADER_STAGE_COMPUTE_BIT;
            break;
        }
    }

    // TODO: configurable max descriptor allocate count.

    uint32_t max_descriptor_allocate_count = 1024;

    // Take a copy of the pointer to the context.
    vulkan_shader_t *out_shader = (vulkan_shader_t *)shader->internal_data;
    out_shader->renderpass      = renderpass;

    // Build out the configuration.
    out_shader->config.max_desc_set_count = max_descriptor_allocate_count;

    // Shader stages. Parse out the flags.
    memory_zero(out_shader->config.stages,
                sizeof(vulkan_shader_stage_config_t) *
                    VULKAN_SHADER_MAX_STAGES);
    out_shader->config.stage_count = 0;
    // Iterate provided stages.
    for (uint32_t i = 0; i < stage_count; i++) {
        // Make sure there is room enough to add the stage.
        if (out_shader->config.stage_count + 1 > VULKAN_SHADER_MAX_STAGES) {
            ar_ERROR("Shaders may have a maximum of %d stages",
                     VULKAN_SHADER_MAX_STAGES);
            return false;
        }

        // Make sure the stage is a supported one.
        VkShaderStageFlagBits stage_flag;
        switch (stages[i]) {
        case SHADER_STAGE_VERTEX:
            stage_flag = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case SHADER_STAGE_FRAGMENT:
            stage_flag = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        default:
            // Go to the next type.
            ar_ERROR("vulkan_shader_create: Unsupported shader stage flagged: "
                     "%d. Stage ignored.",
                     stages[i]);
            continue;
        }

        // Set the stage and bump the counter.
        out_shader->config.stages[out_shader->config.stage_count].stage =
            stage_flag;
        string_ncopy(out_shader->config.stages[out_shader->config.stage_count]
                         .filename,
                     stage_filenames[i], 255);
        out_shader->config.stage_count++;
    }

    // Zero out arrays and counts.
    memory_zero(out_shader->config.desc_set,
                sizeof(vulkan_descriptor_set_config_t) * 2);

    // Attributes array.
    memory_zero(out_shader->config.attr,
                sizeof(VkVertexInputAttributeDescription) *
                    VULKAN_SHADER_MAX_ATTRIBUTES);

    // For now, shaders will only ever have these 2 types of descriptor pools.
    out_shader->config.pool_size[0] =
        (VkDescriptorPoolSize){VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                               1024}; // HACK: max number of ubo descriptor
                                      // sets.
    out_shader->config.pool_size[1] =
        (VkDescriptorPoolSize){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               4096}; // HACK: max number of image sampler
                                      // descriptor sets.

    // Global descriptor set config.
    vulkan_descriptor_set_config_t global_descriptor_set_config = {};

    // UBO is always available and first.
    global_descriptor_set_config.bindings[BINDING_INDEX_UBO].binding =
        BINDING_INDEX_UBO;
    global_descriptor_set_config.bindings[BINDING_INDEX_UBO].descriptorCount =
        1;
    global_descriptor_set_config.bindings[BINDING_INDEX_UBO].descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_descriptor_set_config.bindings[BINDING_INDEX_UBO].stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    global_descriptor_set_config.bind_count++;

    out_shader->config.desc_set[DESC_SET_INDEX_GLOBAL] =
        global_descriptor_set_config;
    out_shader->config.desc_set_count++;
    if (shader->use_instances) {
        // If using instances, add a second descriptor set.
        vulkan_descriptor_set_config_t instance_descriptor_set_config = {};

        // Add a UBO to it, as instances should always have one available.
        // NOTE: Might be a good idea to only add this if it is going to be
        // used...
        instance_descriptor_set_config.bindings[BINDING_INDEX_UBO].binding =
            BINDING_INDEX_UBO;
        instance_descriptor_set_config.bindings[BINDING_INDEX_UBO]
            .descriptorCount = 1;
        instance_descriptor_set_config.bindings[BINDING_INDEX_UBO]
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        instance_descriptor_set_config.bindings[BINDING_INDEX_UBO].stageFlags =
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        instance_descriptor_set_config.bind_count++;

        out_shader->config.desc_set[DESC_SET_INDEX_INSTANCE] =
            instance_descriptor_set_config;
        out_shader->config.desc_set_count++;
    }

    // Invalidate all instance states.
    // TODO: dynamic
    for (uint32_t i = 0; i < 1024; ++i) {
        out_shader->instance_states[i].id = INVALID_ID;
    }

    return true;
}

void vk_backend_shader_shut(struct shader_t *shader) {
    if (shader && shader->internal_data) {
        vulkan_shader_t *shaders = shader->internal_data;
        if (!shaders) {
            ar_ERROR("vulkan_renderer_shader_destroy requires a valid pointer "
                     "to a shader.");
            return;
        }

        VkDevice               logical_device = context.device.logic_dev;
        VkAllocationCallbacks *vk_allocator   = context.alloc;

        // Descriptor set layouts.
        for (uint32_t i = 0; i < shaders->config.desc_set_count; ++i) {
            if (shaders->desc_set_layouts[i]) {
                vkDestroyDescriptorSetLayout(logical_device,
                                             shaders->desc_set_layouts[i],
                                             vk_allocator);
                shaders->desc_set_layouts[i] = 0;
            }
        }

        // Descriptor pool
        if (shaders->desc_pool) {
            vkDestroyDescriptorPool(logical_device, shaders->desc_pool,
                                    vk_allocator);
        }

        // Uniform buffer.
        vk_buffer_unlock_mem(&context, &shaders->uniform_buffer);
        shaders->map_uni_buffer_block = 0;
        vk_buffer_shut(&context, &shaders->uniform_buffer);

        // Pipeline
        vk_pipeline_shut(&context, &shaders->pipeline);

        // Shader modules
        for (uint32_t i = 0; i < shaders->config.stage_count; ++i) {
            vkDestroyShaderModule(context.device.logic_dev,
                                  shaders->stages[i].handle, context.alloc);
        }

        // Destroy the configuration.
        memory_zero(&shaders->config, sizeof(vulkan_shader_config_t));

        // Free the internal data memory.
        memory_free(shader->internal_data, sizeof(vulkan_shader_t),
                    MEMTAG_RENDERER);
        shader->internal_data = 0;
    }
}

b8 vk_backend_shader_init(struct shader_t *shader) {
    VkDevice               logical_device = context.device.logic_dev;
    VkAllocationCallbacks *vk_allocator   = context.alloc;
    vulkan_shader_t       *s = (vulkan_shader_t *)shader->internal_data;

    // Create a module for each stage.
    memory_zero(s->stages,
                sizeof(vulkan_shader_stage_t) * VULKAN_SHADER_MAX_STAGES);
    for (uint32_t i = 0; i < s->config.stage_count; ++i) {
        if (!create_module(s, s->config.stages[i], &s->stages[i])) {
            ar_ERROR("Unable to create %s shader module for '%s'. Shader will "
                     "be destroyed.",
                     s->config.stages[i].filename, shader->name);
            return false;
        }
    }

    // Static lookup table for our types->Vulkan ones.
    static VkFormat *types = 0;
    static VkFormat  t[11];
    if (!types) {
        t[SHADER_ATTR_FLOAT32]   = VK_FORMAT_R32_SFLOAT;
        t[SHADER_ATTR_FLOAT32_2] = VK_FORMAT_R32G32_SFLOAT;
        t[SHADER_ATTR_FLOAT32_3] = VK_FORMAT_R32G32B32_SFLOAT;
        t[SHADER_ATTR_FLOAT32_4] = VK_FORMAT_R32G32B32A32_SFLOAT;
        t[SHADER_ATTR_INT8]      = VK_FORMAT_R8_SINT;
        t[SHADER_ATTR_UINT8]     = VK_FORMAT_R8_UINT;
        t[SHADER_ATTR_INT16]     = VK_FORMAT_R16_SINT;
        t[SHADER_ATTR_UINT16]    = VK_FORMAT_R16_UINT;
        t[SHADER_ATTR_INT32]     = VK_FORMAT_R32_SINT;
        t[SHADER_ATTR_UINT32]    = VK_FORMAT_R32_UINT;
        types                    = t;
    }

    // Process attributes
    uint32_t attribute_count = dyn_array_length(shader->attributes);
    uint32_t offset          = 0;
    for (uint32_t i = 0; i < attribute_count; ++i) {
        // Setup the new attribute.
        VkVertexInputAttributeDescription attribute;
        attribute.location = i;
        attribute.binding  = 0;
        attribute.offset   = offset;
        attribute.format   = types[shader->attributes[i].type];

        // Push into the config's attribute collection and add to the stride.
        s->config.attr[i]  = attribute;

        offset += shader->attributes[i].size;
    }

    // Process uniforms.
    uint32_t uniform_count = dyn_array_length(shader->uniforms);
    for (uint32_t i = 0; i < uniform_count; ++i) {
        // For samplers, the descriptor bindings need to be updated. Other types
        // of uniforms don't need anything to be done here.
        if (shader->uniforms[i].type == SHADER_UNIFORM_SAMPLER) {
            const uint32_t set_index =
                (shader->uniforms[i].scope == SHADER_SCOPE_GLOBAL
                     ? DESC_SET_INDEX_GLOBAL
                     : DESC_SET_INDEX_INSTANCE);
            vulkan_descriptor_set_config_t *set_config =
                &s->config.desc_set[set_index];
            if (set_config->bind_count < 2) {
                // There isn't a binding yet, meaning this is the first sampler
                // to be added. Create the binding with a single descriptor for
                // this sampler.
                set_config->bindings[BINDING_INDEX_SAMPLER].binding =
                    BINDING_INDEX_SAMPLER; // Always going to be the second one.
                set_config->bindings[BINDING_INDEX_SAMPLER].descriptorCount =
                    1; // Default to 1, will increase with each sampler added to
                       // the appropriate level.
                set_config->bindings[BINDING_INDEX_SAMPLER].descriptorType =
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                set_config->bindings[BINDING_INDEX_SAMPLER].stageFlags =
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                set_config->bind_count++;
            } else {
                // There is already a binding for samplers, so just add a
                // descriptor to it. Take the current descriptor count as the
                // location and increment the number of descriptors.
                set_config->bindings[BINDING_INDEX_SAMPLER].descriptorCount++;
            }
        }
    }

    // Descriptor pool.
    VkDescriptorPoolCreateInfo pool_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes    = s->config.pool_size;
    pool_info.maxSets       = s->config.max_desc_set_count;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    // Create descriptor pool.
    VkResult result         = vkCreateDescriptorPool(logical_device, &pool_info,
                                                     vk_allocator, &s->desc_pool);
    if (!vk_result_is_success(result)) {
        ar_ERROR("vulkan_shader_initialize failed creating descriptor pool: "
                 "'%s'", vk_result_string(result, true));
        return false;
    }

    // Create descriptor set layouts.
    memory_zero(s->desc_set_layouts, s->config.desc_set_count);
    for (uint32_t i = 0; i < s->config.desc_set_count; ++i) {
        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = s->config.desc_set[i].bind_count;
        layout_info.pBindings    = s->config.desc_set[i].bindings;
        result =
            vkCreateDescriptorSetLayout(logical_device, &layout_info,
                                        vk_allocator, &s->desc_set_layouts[i]);
        if (!vk_result_is_success(result)) {
            ar_ERROR("vulkan_shader_initialize failed creating descriptor "
                     "pool: '%s'", vk_result_string(result, true));
            return false;
        }
    }

    // TODO: This feels wrong to have these here, at least in this fashion.
    // Should probably Be configured to pull from someplace instead. Viewport.
    VkViewport viewport;
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)context.framebuffer_w;
    viewport.height   = (float)context.framebuffer_h;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width                = context.framebuffer_w;
    scissor.extent.height               = context.framebuffer_h;

    VkPipelineShaderStageCreateInfo
        stage_create_infos[VULKAN_SHADER_MAX_STAGES];
    memory_zero(stage_create_infos, sizeof(VkPipelineShaderStageCreateInfo) *
                                        VULKAN_SHADER_MAX_STAGES);
    for (uint32_t i = 0; i < s->config.stage_count; ++i) {
        stage_create_infos[i] = s->stages[i].shader_stg_cr_info;
    }

    b8 pipeline_result =
        vk_pipeline_init(&context, s->renderpass, shader->attr_stride,
                         dyn_array_length(shader->attributes),
                         s->config.desc_set_count, s->config.stage_count,
                         s->config.attr, s->desc_set_layouts,
                         stage_create_infos, viewport, scissor, false, true,
                         shader->push_const_range_count,
                         shader->push_const_ranges, &s->pipeline);

    if (!pipeline_result) {
        ar_ERROR("Failed to load graphics pipeline for object shader.");
        return false;
    }

    // Grab the UBO alignment requirement from the device.
    shader->req_ubo_alignment =
        context.device.properties.limits.minUniformBufferOffsetAlignment;

    // Make sure the UBO is aligned according to device requirements.
    shader->global_ubo_stride =
        get_aligned(shader->global_ubo_size, shader->req_ubo_alignment);
    shader->ubo_stride =
        get_aligned(shader->ubo_size, shader->req_ubo_alignment);

    // Uniform  buffer.
    // TODO: max count should be configurable, or perhaps long term support of
    // buffer resizing.
    uint64_t total_buffer_size =
        shader->global_ubo_stride +
        (shader->ubo_stride * VULKAN_MATERIAL_MAX_COUNT); // global + (locals)
    if (!vk_buffer_init(&context, total_buffer_size,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        true, true, &s->uniform_buffer)) {
        ar_ERROR("Vulkan buffer creation failed for object shader.");
        return false;
    }

    // Allocate space for the global UBO, whcih should occupy the _stride_
    // space, _not_ the actual size used.
    if (!vk_buffer_allocate(&s->uniform_buffer, shader->global_ubo_stride,
                            &shader->global_ubo_offset)) {
        ar_ERROR("Failed to allocate space for the uniform buffer!");
        return false;
    }

    // Map the entire buffer's memory.
    s->map_uni_buffer_block =
        vk_buffer_lock_mem(&context, &s->uniform_buffer, 0,
                           VK_WHOLE_SIZE /*total_buffer_size*/, 0);

    // Allocate global descriptor sets, one per frame. Global is always the
    // first set.
    VkDescriptorSetLayout global_layouts[4] =
        {s->desc_set_layouts[DESC_SET_INDEX_GLOBAL],
         s->desc_set_layouts[DESC_SET_INDEX_GLOBAL],
         s->desc_set_layouts[DESC_SET_INDEX_GLOBAL],
         s->desc_set_layouts[DESC_SET_INDEX_GLOBAL]};

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = s->desc_pool;
    alloc_info.descriptorSetCount = 4;
    alloc_info.pSetLayouts        = global_layouts;
    VK_CHECK(vkAllocateDescriptorSets(context.device.logic_dev, &alloc_info,
                                      s->global_desc_sets));

    return true;
}

#ifdef _DEBUG
#define SHADER_VERIFY_SHADER_ID(shader_id)                                     \
    if (shader_id == INVALID_ID ||                                             \
        context.shaders[shader_id].id == INVALID_ID) {                         \
        return false;                                                          \
    }
#else
#define SHADER_VERIFY_SHADER_ID(shader_id) // do nothing
#endif

b8 vk_backend_shader_use(struct shader_t *shader) {
    vulkan_shader_t *s = shader->internal_data;
    vk_pipeline_bind(&context.graphic_comm_buffer[context.image_idx],
                     VK_PIPELINE_BIND_POINT_GRAPHICS, &s->pipeline);
    return true;
}

b8 vk_backend_shader_bind_globals(struct shader_t* s) {
	if (!s)
		return false;

	s->bound_ubo_offset = s->global_ubo_offset;
	return true;
}

b8 vk_backend_shader_bind_instance(struct shader_t *s, uint32_t instance_id) {
    if (!s) {
        ar_ERROR("vk_backend_shader_bind_instance - require a valid pointer to "
                 "shader");
        return false;
    }

    vulkan_shader_t                *internal = s->internal_data;
    vulkan_shader_instance_state_t *obj_state =
        &internal->instance_states[instance_id];
    s->bound_ubo_offset = obj_state->offset;
    return true;
}

b8 vk_backend_shader_apply_globals(struct shader_t* s) {
	uint32_t img_idx = context.image_idx;
	vulkan_shader_t *internal = s->internal_data;
	VkCommandBuffer combuff = context.graphic_comm_buffer[img_idx].handle;
	VkDescriptorSet global_desc = internal->global_desc_sets[img_idx];

    if (!internal->desc_updated[img_idx]) {
        VkDescriptorBufferInfo bufferInfo;
        bufferInfo.buffer              = internal->uniform_buffer.handle;
        bufferInfo.offset              = s->global_ubo_offset;
        bufferInfo.range               = s->global_ubo_stride;

        VkWriteDescriptorSet ubo_write = {};
        ubo_write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ubo_write.dstSet               = internal->global_desc_sets[img_idx];
        ubo_write.dstBinding           = 0;
        ubo_write.dstArrayElement      = 0;
        ubo_write.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_write.descriptorCount      = 1;
        ubo_write.pBufferInfo          = &bufferInfo;

        VkWriteDescriptorSet descriptor_writes[2];
        descriptor_writes[0] = ubo_write;

        uint32_t global_set_binding_count =
            internal->config.desc_set[DESC_SET_INDEX_GLOBAL].bind_count;
        if (global_set_binding_count > 1) {
            // TODO: There are samplers to be written. Support this.
            global_set_binding_count = 1;
            ar_ERROR("Global image samplers are not yet supported.");

            // VkWriteDescriptorSet sampler_write =
            // {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}; descriptor_writes[1] =
            // ...
        }

        vkUpdateDescriptorSets(context.device.logic_dev,
                               global_set_binding_count, descriptor_writes, 0,
                               0);
    }

    vkCmdBindDescriptorSets(combuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            internal->pipeline.pipe_layout, 0, 1,
                            &global_desc, 0, 0);
    return true;
}

b8 vk_backend_shader_apply_instance(struct shader_t *s) {
    if (!s->use_instances) {
        ar_ERROR("This shader does not use instances.");
        return false;
    }
    vulkan_shader_t *internal    = s->internal_data;
    uint32_t         image_index = context.image_idx;
    VkCommandBuffer  command_buffer =
        context.graphic_comm_buffer[image_index].handle;

    // Obtain instance data.
    vulkan_shader_instance_state_t *object_state =
        &internal->instance_states[s->bound_inst_id];
    VkDescriptorSet object_descriptor_set =
        object_state->desc_set_state.desc_sets[image_index];

    // TODO: if needs update
    VkWriteDescriptorSet
        descriptor_writes[2]; // Always a max of 2 descriptor sets.
    memory_zero(descriptor_writes, sizeof(VkWriteDescriptorSet) * 2);
    uint32_t descriptor_count = 0;
    uint32_t descriptor_index = 0;

    // Descriptor 0 - Uniform buffer
    // Only do this if the descriptor has not yet been updated.
    uint8_t *instance_ubo_generation =
        &(object_state->desc_set_state.desc_states[descriptor_index]
              .gen[image_index]);
    // TODO: determine if update is required.
    if (*instance_ubo_generation ==
        INVALID_ID_U8 /*|| *global_ubo_generation != material->generation*/) {
        VkDescriptorBufferInfo buffer_info;
        buffer_info.buffer                  = internal->uniform_buffer.handle;
        buffer_info.offset                  = object_state->offset;
        buffer_info.range                   = s->ubo_stride;

        VkWriteDescriptorSet ubo_descriptor = {};
        ubo_descriptor.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ubo_descriptor.dstSet          = object_descriptor_set;
        ubo_descriptor.dstBinding      = descriptor_index;
        ubo_descriptor.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_descriptor.descriptorCount = 1;
        ubo_descriptor.pBufferInfo     = &buffer_info;

        descriptor_writes[descriptor_count] = ubo_descriptor;
        descriptor_count++;

        // Update the frame generation. In this case it is only needed once
        // since this is a buffer.
        *instance_ubo_generation =
            1; // material->generation; TODO: some generation from... somewhere
    }
    descriptor_index++;

    // Samplers will always be in the binding. If the binding count is less than
    // 2, there are no samplers.
    if (internal->config.desc_set[DESC_SET_INDEX_INSTANCE].bind_count > 1) {
        // Iterate samplers.
        uint32_t total_sampler_count =
            internal->config.desc_set[DESC_SET_INDEX_INSTANCE]
                .bindings[BINDING_INDEX_SAMPLER]
                .descriptorCount;
        uint32_t              update_sampler_count = 0;
        VkDescriptorImageInfo image_infos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES];
        for (uint32_t i = 0; i < total_sampler_count; ++i) {
            // TODO: only update in the list if actually needing an update.
            texture_t *t = internal->instance_states[s->bound_inst_id]
                               .instance_textures[i];
            vulkan_texture_data_t *internal_data =
                (vulkan_texture_data_t *)t->internal_data;
            image_infos[i].imageLayout =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[i].imageView = internal_data->image.image_view;
            image_infos[i].sampler   = internal_data->sampler;

            // TODO: change up descriptor state to handle this properly.
            // Sync frame generation if not using a default texture.
            // if (t->generation != INVALID_ID) {
            //     *descriptor_generation = t->generation;
            //     *descriptor_id = t->id;
            // }

            update_sampler_count++;
        }

        VkWriteDescriptorSet sampler_descriptor = {};
        sampler_descriptor.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sampler_descriptor.dstSet     = object_descriptor_set;
        sampler_descriptor.dstBinding = descriptor_index;
        sampler_descriptor.descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_descriptor.descriptorCount  = update_sampler_count;
        sampler_descriptor.pImageInfo       = image_infos;

        descriptor_writes[descriptor_count] = sampler_descriptor;
        descriptor_count++;
    }

    if (descriptor_count > 0) {
        vkUpdateDescriptorSets(context.device.logic_dev, descriptor_count,
                               descriptor_writes, 0, 0);
    }

    // Bind the descriptor set to be updated, or in case the shader changed.
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            internal->pipeline.pipe_layout, 1, 1,
                            &object_descriptor_set, 0, 0);
    return true;
}

b8 vk_backend_shader_acquire_inst_resc(struct shader_t *s,
                                       uint32_t        *out_instance_id) {
    vulkan_shader_t *internal = s->internal_data;
    // TODO: dynamic
    *out_instance_id          = INVALID_ID;
    for (uint32_t i = 0; i < 1024; ++i) {
        if (internal->instance_states[i].id == INVALID_ID) {
            internal->instance_states[i].id = i;
            *out_instance_id                = i;
            break;
        }
    }
    if (*out_instance_id == INVALID_ID) {
        ar_ERROR("vulkan_shader_acquire_instance_resources failed to acquire "
                 "new id");
        return false;
    }

    vulkan_shader_instance_state_t *instance_state =
        &internal->instance_states[*out_instance_id];
    uint32_t instance_texture_count =
        internal->config.desc_set[DESC_SET_INDEX_INSTANCE]
            .bindings[BINDING_INDEX_SAMPLER]
            .descriptorCount;
    // Wipe out the memory for the entire array, even if it isn't all used.
    instance_state->instance_textures =
        memory_alloc(sizeof(texture_t *) * s->inst_texture_count, MEMTAG_ARRAY);
    texture_t *default_texture = texture_sys_get_default_tex();
    // Set all the texture pointers to default until assigned.
    for (uint32_t i = 0; i < instance_texture_count; ++i) {
        instance_state->instance_textures[i] = default_texture;
    }

    // Allocate some space in the UBO - by the stride, not the size.
    uint64_t size = s->ubo_stride;
    if (!vk_buffer_allocate(&internal->uniform_buffer, size,
                            &instance_state->offset)) {
        ar_ERROR("vulkan_material_shader_acquire_resources failed to acquire "
                 "ubo space");
        return false;
    }

    vulkan_shader_desc_set_state_t *set_state = &instance_state->desc_set_state;

    // Each descriptor binding in the set
    uint32_t binding_count =
        internal->config.desc_set[DESC_SET_INDEX_INSTANCE].bind_count;
    memory_zero(set_state->desc_states,
                sizeof(vulkan_descriptor_state_t) * VULKAN_SHADER_MAX_BINDING);
    for (uint32_t i = 0; i < binding_count; ++i) {
        for (uint32_t j = 0; j < 4; ++j) {
            set_state->desc_states[i].gen[j] = INVALID_ID_U8;
            set_state->desc_states[i].ids[j] = INVALID_ID;
        }
    }

    // Allocate 3 descriptor sets (one per frame).
    VkDescriptorSetLayout layouts[4] =
        {internal->desc_set_layouts[DESC_SET_INDEX_INSTANCE],
         internal->desc_set_layouts[DESC_SET_INDEX_INSTANCE],
         internal->desc_set_layouts[DESC_SET_INDEX_INSTANCE],
         internal->desc_set_layouts[DESC_SET_INDEX_INSTANCE]};

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = internal->desc_pool;
    alloc_info.descriptorSetCount = 3;
    alloc_info.pSetLayouts        = layouts;
    VkResult result =
        vkAllocateDescriptorSets(context.device.logic_dev, &alloc_info,
                                 instance_state->desc_set_state.desc_sets);
    if (result != VK_SUCCESS) {
        ar_ERROR("Error allocating instance descriptor sets in shader: '%s'.",
                 vk_result_string(result, true));

        return false;
    }

    return true;
}

b8 vk_backend_shader_release_inst_resc(struct shader_t *s,
                                       uint32_t         instance_id) {
    vulkan_shader_t                *internal = s->internal_data;
    vulkan_shader_instance_state_t *instance_state =
        &internal->instance_states[instance_id];

    // Wait for any pending operations using the descriptor set to finish.
    vkDeviceWaitIdle(context.device.logic_dev);

    // Free 3 descriptor sets (one per frame)
    VkResult result =
        vkFreeDescriptorSets(context.device.logic_dev, internal->desc_pool, 4,
                             instance_state->desc_set_state.desc_sets);
    if (result != VK_SUCCESS) {
        ar_ERROR("Error freeing object shader descriptor sets!");
    }

    // Destroy descriptor states.
    memory_zero(instance_state->desc_set_state.desc_sets,
                sizeof(vulkan_descriptor_state_t) * VULKAN_SHADER_MAX_BINDING);

    if (instance_state->instance_textures) {
        memory_free(instance_state->instance_textures,
                    sizeof(texture_t *) * s->inst_texture_count, MEMTAG_ARRAY);
        instance_state->instance_textures = 0;
    }

    vk_buffer_free(&internal->uniform_buffer, s->ubo_stride,
                   instance_state->offset);
    instance_state->offset = INVALID_ID;
    instance_state->id     = INVALID_ID;

    return true;
}

b8 vk_backend_set_uniform(struct shader_t         *fe_shader,
                          struct shader_uniform_t *uniform, const void *value) {
    vulkan_shader_t *internal = fe_shader->internal_data;
    if (uniform->type == SHADER_UNIFORM_SAMPLER) {
        if (uniform->scope == SHADER_SCOPE_GLOBAL) {
            fe_shader->global_textures[uniform->location] = (texture_t *)value;
        } else {
            internal->instance_states[fe_shader->bound_inst_id]
                .instance_textures[uniform->location] = (texture_t *)value;
        }
    } else {
        if (uniform->scope == SHADER_SCOPE_LOCAL) {
            // Is local, using push constants. Do this immediately.
            VkCommandBuffer command_buffer =
                context.graphic_comm_buffer[context.image_idx].handle;
            vkCmdPushConstants(command_buffer, internal->pipeline.pipe_layout,
                               VK_SHADER_STAGE_VERTEX_BIT |
                                   VK_SHADER_STAGE_FRAGMENT_BIT,
                               uniform->offset, uniform->size, value);
        } else {
            // Map the appropriate memory location and copy the data over.
            uint64_t addr = (uint64_t)internal->map_uni_buffer_block;
            addr += fe_shader->bound_ubo_offset + uniform->offset;
            memory_copy((void *)addr, value, uniform->size);
            if (addr) {
            }
        }
    }
    return true;
}
