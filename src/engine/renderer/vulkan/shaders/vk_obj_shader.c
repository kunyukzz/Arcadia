#include "engine/renderer/vulkan/shaders/vk_obj_shader.h"

#include "engine/renderer/vulkan/vk_buffer.h"
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

	/* Global Descriptor */
	VkDescriptorSetLayoutBinding global_desc_set_layout_bind = {};
	global_desc_set_layout_bind.binding = 0;
	global_desc_set_layout_bind.descriptorCount = 1;
    global_desc_set_layout_bind.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_desc_set_layout_bind.pImmutableSamplers = 0;
    global_desc_set_layout_bind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo global_set_layout_info = {};
    global_set_layout_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    global_set_layout_info.bindingCount = 1;
    global_set_layout_info.pBindings = &global_desc_set_layout_bind;

    VK_CHECK(vkCreateDescriptorSetLayout(ctx->device.logic_dev,
                                         &global_set_layout_info, ctx->alloc,
                                         &shader->global_desc_set_layout));

	VkDescriptorPoolSize global_pool_size = {};
	global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	global_pool_size.descriptorCount = ctx->swapchain.image_count;

	VkDescriptorPoolCreateInfo global_pool_info = {};
	global_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	global_pool_info.poolSizeCount = 1;
	global_pool_info.pPoolSizes = &global_pool_size;
	global_pool_info.maxSets = ctx->swapchain.image_count;

    VK_CHECK(vkCreateDescriptorPool(ctx->device.logic_dev, &global_pool_info,
                                    ctx->alloc, &shader->global_desc_pool));

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

	/* Descriptor Set Layout */
#define desc_set_layout_count 1
    VkDescriptorSetLayout layouts[desc_set_layout_count] = {
        shader->global_desc_set_layout};

    VkPipelineShaderStageCreateInfo stg_info[OBJ_SHADER_STAGE_COUNT];
	memory_zero(stg_info, sizeof(stg_info));
	for (uint32_t i = 0; i < OBJ_SHADER_STAGE_COUNT; ++i) {
		stg_info[i].sType = shader->stages[i].shader_stg_cr_info.sType;
		stg_info[i] = shader->stages[i].shader_stg_cr_info;
	}

    if (!vk_pipeline_init(ctx, &ctx->main_render, attr_count, attr_desc,
                          desc_set_layout_count, layouts,
                          OBJ_SHADER_STAGE_COUNT, stg_info, viewport, scissor,
                          false, &shader->pipeline)) {
        ar_ERROR("Failed to load graphic pipeline for object shader");
        return false;
    }

	/* Create Uniform Buffer */
	/*
    uint32_t dev_local_bits = ctx->device.support_dev_local_host_vsb
                                  ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                  : 0;
	*/

    if (!vk_buffer_init(ctx, sizeof(global_uni_obj_t) * ctx->swapchain.image_count,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       true, &shader->global_uni_buffer)) {
        ar_ERROR("Vulkan Buffer creation failed for object shader");
        return false;
    }

    VkDescriptorSetLayout global_layout[4] = {shader->global_desc_set_layout,
                                              shader->global_desc_set_layout,
                                              shader->global_desc_set_layout,
                                              shader->global_desc_set_layout};

    VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = shader->global_desc_pool;
	alloc_info.descriptorSetCount = ctx->swapchain.image_count;
	alloc_info.pSetLayouts = global_layout;

    VK_CHECK(vkAllocateDescriptorSets(ctx->device.logic_dev, &alloc_info,
                                      shader->global_desc_sets));

    return true;
}

void vk_obj_shader_shut(vulkan_context_t *ctx, vulkan_object_shader_t *shader) {
	/* Destroy uniform buffer */
	vk_buffer_shut(ctx, &shader->global_uni_buffer);

	/* Destroy Pipeline */
	vk_pipeline_shut(ctx, &shader->pipeline);

	/* Destroy global descriptor pool */
    vkDestroyDescriptorPool(ctx->device.logic_dev, shader->global_desc_pool,
                            ctx->alloc);
	
	/* Destroy descriptor set layout */
    vkDestroyDescriptorSetLayout(ctx->device.logic_dev,
                                 shader->global_desc_set_layout, ctx->alloc);

    for (uint32_t i = 0; i < OBJ_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(ctx->device.logic_dev, shader->stages[i].handle,
                              ctx->alloc);
        shader->stages[i].handle = 0;
    }
}

void vk_obj_shader_use(vulkan_context_t *ctx, vulkan_object_shader_t *shader) {
	uint32_t image_idx = ctx->image_idx;
    vk_pipeline_bind(&ctx->graphic_comm_buffer[image_idx],
                     VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}

void vk_obj_shader_update_global_state(vulkan_context_t       *ctx,
                                       vulkan_object_shader_t *shader) {
	uint32_t image_idx = ctx->image_idx;
	VkCommandBuffer combuff = ctx->graphic_comm_buffer[image_idx].handle;
	VkDescriptorSet global_desc = shader->global_desc_sets[image_idx];

	/* Config descriptor for index given */
	uint32_t range = sizeof(global_uni_obj_t);
	uint64_t offset = sizeof(global_uni_obj_t) * image_idx;

	vk_buffer_load_data(ctx, &shader->global_uni_buffer, offset, range, 0,
						&shader->global_ubo);

	if (!shader->desc_updated[image_idx]) {	
		
		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = shader->global_uni_buffer.handle;
		buffer_info.offset = offset;
		buffer_info.range = range;

		/* Update descriptor set */
		VkWriteDescriptorSet desc_write = {};
		desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc_write.dstSet = shader->global_desc_sets[image_idx];
		desc_write.dstBinding = 0;
		desc_write.dstArrayElement = 0;
		desc_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desc_write.descriptorCount = 1;
		desc_write.pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(ctx->device.logic_dev, 1, &desc_write, 0, 0);
		shader->desc_updated[image_idx] = true;
	}

    vkCmdBindDescriptorSets(combuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            shader->pipeline.pipe_layout, 0, 1, &global_desc, 0,
                            0);
}

void vk_obj_shader_update_obj(vulkan_context_t       *ctx,
                              vulkan_object_shader_t *shader, mat4 model) {
    uint32_t        image_idx = ctx->image_idx;
    VkCommandBuffer combuff = ctx->graphic_comm_buffer[image_idx].handle;

    vkCmdPushConstants(combuff, shader->pipeline.pipe_layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &model);
}

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


