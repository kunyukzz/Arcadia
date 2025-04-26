#include "engine/renderer/vulkan/shaders/vk_obj_shader.h"

#include "engine/renderer/vulkan/vk_shader_util.h"
#include "engine/renderer/vulkan/vk_pipeline.h"

#include "engine/core/logger.h"
#include "engine/memory/memory.h"
#include "engine/math/math_type.h"

#define BUILTIN_SHADER_OBJ "Builtin.ObjShader"

b8   vk_obj_shader_init(vulkan_context_t *ctx, vulkan_object_shader_t *shader) {
    char stg_type_str[OBJ_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits stg_type[OBJ_SHADER_STAGE_COUNT] =
        {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    for (uint32_t i = 0; i < OBJ_SHADER_STAGE_COUNT; ++i) {
        if (!vk_shader_module_init(ctx, BUILTIN_SHADER_OBJ, stg_type_str[i],
                                   stg_type[i], i, shader->stages)) {
            ar_ERROR("Unable to create %s shader module for '%s'.",
                     stg_type_str[i], BUILTIN_SHADER_OBJ);
            return false;
        }
    }

	/* Pipeline Creation */
	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)ctx->framebuffer_w;
	viewport.height = (float)ctx->framebuffer_h;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset.x = scissor.offset.y = 0;
	scissor.extent.width = ctx->swapchain.extents.width;
	scissor.extent.height = ctx->swapchain.extents.height;

	/* Attributes */
	uint32_t offset = 0;
	const int32_t attr_count = 1;
	VkVertexInputAttributeDescription attr_desc[1];

	/* Position */
    VkFormat formats[1] = {VK_FORMAT_R32G32B32_SFLOAT};
    uint64_t sizes[1]   = {sizeof(vec3)};

    for (uint32_t i = 0; i < attr_count; ++i) {
		attr_desc[i].binding = 0;
		attr_desc[i].location = i;
		attr_desc[i].format = formats[i];
		attr_desc[i].offset = offset;
		offset += sizes[i];
	}

	VkPipelineShaderStageCreateInfo stg_info[OBJ_SHADER_STAGE_COUNT];
	memory_zero(stg_info, sizeof(stg_info));
	for (uint32_t i = 0; i < OBJ_SHADER_STAGE_COUNT; ++i) {
		stg_info[i].sType = shader->stages[i].shader_stg_cr_info.sType;
		stg_info[i] = shader->stages[i].shader_stg_cr_info;
	}

    if (!vk_pipeline_init(ctx, &ctx->main_render, attr_count, attr_desc, 0, 0,
                          OBJ_SHADER_STAGE_COUNT, stg_info, viewport, scissor,
                          false, &shader->pipeline)) {
		ar_ERROR("Failed to load graphic pipeline for object shader");
		return false;
    }

    return true;
}

void vk_obj_shader_shut(vulkan_context_t *ctx, vulkan_object_shader_t *shader) {
	vk_pipeline_shut(ctx, &shader->pipeline);

    for (uint32_t i = 0; i < OBJ_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(ctx->device.logic_dev, shader->stages[i].handle,
                              ctx->alloc);
        shader->stages[i].handle = 0;
    }
}

void vk_obj_shader_use(vulkan_context_t *ctx, vulkan_object_shader_t *shader) {
	(void)ctx;
	(void)shader;
}

/*
void vk_obj_shader_update_global_state(vulkan_context_t *ctx,
vulkan_object_shader_t *shader, float delta_time); void
vk_obj_shader_update_obj(vulkan_context_t *ctx, vulkan_object_shader_t *shader,
geo_render_data_t data);
*/

b8   vk_obj_shader_acquire_rsc(vulkan_context_t       *ctx,
                               vulkan_object_shader_t *shader, uint32_t *obj_id) {

	(void)ctx;
	(void)shader;
	(void)obj_id;
	return true;
}

void vk_obj_shader_release_rsc(vulkan_context_t       *ctx,
                               vulkan_object_shader_t *shader,
                               uint32_t               *obj_id) {
	(void)ctx;
	(void)shader;
	(void)obj_id;
}


