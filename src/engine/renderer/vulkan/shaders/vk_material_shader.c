#include "engine/renderer/vulkan/shaders/vk_material_shader.h"

#include "engine/define.h"
#include "engine/renderer/vulkan/vk_buffer.h"
#include "engine/renderer/vulkan/vk_shader_util.h"
#include "engine/renderer/vulkan/vk_pipeline.h"
#include "engine/renderer/vulkan/vk_type.h"
#include "engine/resources/resc_type.h"
#include "engine/systems/texture_sys.h"

//#include "engine/container/dyn_array.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"
#include "engine/math/math_type.h"
#include <vulkan/vulkan_core.h>

#define BUILTIN_MATERIAL_SHADER "Builtin.MaterialShader"

b8 vk_material_shader_init(vulkan_context_t         *ctx,
                           vulkan_material_shader_t *shader) {
    char stg_type_str[MATERIAL_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits stg_type[MATERIAL_SHADER_STAGE_COUNT] =
        {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    for (uint32_t i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i) {
        if (!vk_shader_module_init(ctx, BUILTIN_MATERIAL_SHADER, stg_type_str[i],
                                   stg_type[i], i, shader->stages)) {
            ar_ERROR("Unable to create %s shader module for '%s'.",
                     stg_type_str[i], BUILTIN_MATERIAL_SHADER);
            return false;
        }
    }

	/* ========================== Global Descriptor ============================ */
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

    shader->sampler_uses[0] = TEXTURE_USE_MAP_DIFFUSE;
	/* ========================== Object Descriptor ============================ */

    VkDescriptorType desc_types[VULKAN_MATERIAL_SHADER_DESC_COUNT] = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        // Binding 0 - uniform buffer
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER // Binding 1 - diffuse sampler
    };

    VkDescriptorSetLayoutBinding binds[VULKAN_MATERIAL_SHADER_DESC_COUNT];
    memory_zero(&binds, sizeof(VkDescriptorSetLayoutBinding) *
                            VULKAN_MATERIAL_SHADER_DESC_COUNT);

    for (uint32_t i = 0; i < VULKAN_MATERIAL_SHADER_DESC_COUNT; ++i) {
		binds[i].binding = i;
		binds[i].descriptorCount = 1;
		binds[i].descriptorType = desc_types[i];
		binds[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}

	VkDescriptorSetLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = VULKAN_MATERIAL_SHADER_DESC_COUNT;
	layout_info.pBindings = binds;

    VK_CHECK(vkCreateDescriptorSetLayout(ctx->device.logic_dev, &layout_info,
                                         ctx->alloc,
                                         &shader->obj_desc_set_layout));

	/* Local Object descriptor pool */
	VkDescriptorPoolSize obj_pool_size[2];
	// This for unifom buffer - binding 0
	obj_pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	obj_pool_size[0].descriptorCount = VULKAN_MATERIAL_MAX_COUNT;
	// This for image sampler - binding 1
	obj_pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	obj_pool_size[1].descriptorCount = VULKAN_MATERIAL_SHADER_SAMPLER_COUNT * VULKAN_MATERIAL_MAX_COUNT;

	VkDescriptorPoolCreateInfo obj_pool_info = {};
	obj_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	obj_pool_info.poolSizeCount = 2;
	obj_pool_info.pPoolSizes = obj_pool_size;
	obj_pool_info.maxSets = VULKAN_MATERIAL_MAX_COUNT;
    obj_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VK_CHECK(vkCreateDescriptorPool(ctx->device.logic_dev, &obj_pool_info,
                                    ctx->alloc, &shader->obj_desc_pool));

	/* ============================ Pipeline Creation ============================ */
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
#define ATTRIBUTE_COUNT 2
	uint32_t offset = 0;
	VkVertexInputAttributeDescription attr_desc[ATTRIBUTE_COUNT];

	// Position & Texture Coordinate
    VkFormat formats[ATTRIBUTE_COUNT] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
    uint64_t sizes[ATTRIBUTE_COUNT]   = {sizeof(vec3), sizeof(vec2)};

    for (uint32_t i = 0; i < ATTRIBUTE_COUNT; ++i) {
		attr_desc[i].binding = 0;
		attr_desc[i].location = i;
		attr_desc[i].format = formats[i];
		attr_desc[i].offset = offset;
		offset += sizes[i];
	}

	/* Descriptor Set Layout */
#define desc_set_layout_count 2
    VkDescriptorSetLayout layouts[desc_set_layout_count] =
        {shader->global_desc_set_layout, shader->obj_desc_set_layout};

    VkPipelineShaderStageCreateInfo stg_info[MATERIAL_SHADER_STAGE_COUNT];
	memory_zero(stg_info, sizeof(stg_info));
	for (uint32_t i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i) {
		stg_info[i].sType = shader->stages[i].shader_stg_cr_info.sType;
		stg_info[i] = shader->stages[i].shader_stg_cr_info;
	}

    if (!vk_pipeline_init(ctx, &ctx->main_render, ATTRIBUTE_COUNT, sizeof(vertex_3d),
                          desc_set_layout_count, MATERIAL_SHADER_STAGE_COUNT, attr_desc, 
                          layouts, stg_info, viewport, scissor,
                          false, true, &shader->pipeline)) {
        ar_ERROR("Failed to load graphic pipeline for object shader");
        return false;
    }

    if (!vk_buffer_init(ctx, sizeof(vulkan_material_shader_global_ubo_t),
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        true, &shader->global_uni_buffer)) {
        ar_ERROR("Vulkan Buffer creation failed for object shader");
        return false;
    }

	VkDescriptorSetLayout global_layout[4] = {
		shader->global_desc_set_layout,
		shader->global_desc_set_layout,
		shader->global_desc_set_layout,
        shader->global_desc_set_layout
	};

    VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = shader->global_desc_pool;
	alloc_info.descriptorSetCount = 4;
	alloc_info.pSetLayouts = global_layout;

    VK_CHECK(vkAllocateDescriptorSets(ctx->device.logic_dev, &alloc_info,
                                      shader->global_desc_sets));

    if (!vk_buffer_init(ctx,
                        sizeof(vulkan_material_shader_instance_ubo_t) *
                            VULKAN_MATERIAL_MAX_COUNT,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        true, &shader->obj_uni_buffer)) {
        ar_ERROR("Material instance create failed for shader");
        return false;
    }

    return true;
}

void vk_material_shader_shut(vulkan_context_t *ctx, vulkan_material_shader_t *shader) {
	/* This was for destroyin the array value of global descriptor sets.*/
    vkDestroyDescriptorPool(ctx->device.logic_dev, shader->obj_desc_pool,
                            ctx->alloc);
    vkDestroyDescriptorSetLayout(ctx->device.logic_dev,
                                 shader->obj_desc_set_layout, ctx->alloc);

    /* Destroy uniform buffer */
	vk_buffer_shut(ctx, &shader->global_uni_buffer);
	vk_buffer_shut(ctx, &shader->obj_uni_buffer);

	/* Destroy Pipeline */
	vk_pipeline_shut(ctx, &shader->pipeline);

	/* Destroy global descriptor pool */
    vkDestroyDescriptorPool(ctx->device.logic_dev, shader->global_desc_pool,
                            ctx->alloc);
	
	/* Destroy descriptor set layout */
    vkDestroyDescriptorSetLayout(ctx->device.logic_dev,
                                 shader->global_desc_set_layout, ctx->alloc);

    for (uint32_t i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(ctx->device.logic_dev, shader->stages[i].handle,
                              ctx->alloc);
        shader->stages[i].handle = 0;
    }
}

void vk_material_shader_use(vulkan_context_t *ctx, vulkan_material_shader_t *shader) {
	uint32_t image_idx = ctx->image_idx;
    vk_pipeline_bind(&ctx->graphic_comm_buffer[image_idx],
                     VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}

void vk_material_shader_update_global_state(vulkan_context_t         *ctx,
                                            vulkan_material_shader_t *shader,
                                            float delta_time) {
    (void)delta_time;
    uint32_t        image_idx   = ctx->image_idx;
    VkCommandBuffer combuff     = ctx->graphic_comm_buffer[image_idx].handle;
    VkDescriptorSet global_desc = shader->global_desc_sets[image_idx];

    /* Config descriptor for index given */
    uint32_t range              = sizeof(vulkan_material_shader_global_ubo_t);
    uint64_t offset             = 0;

    vk_buffer_load_data(ctx, &shader->global_uni_buffer, offset, range, 0,
                        &shader->global_ubo);

    if (!shader->desc_updated[image_idx]) {
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer                 = shader->global_uni_buffer.handle;
        buffer_info.offset                 = offset;
        buffer_info.range                  = range;

        /* Update descriptor set */
        VkWriteDescriptorSet desc_write    = {};
        desc_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_write.dstSet          = shader->global_desc_sets[image_idx];
        desc_write.dstBinding      = 0;
        desc_write.dstArrayElement = 0;
        desc_write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc_write.descriptorCount = 1;
        desc_write.pBufferInfo     = &buffer_info;

        vkUpdateDescriptorSets(ctx->device.logic_dev, 1, &desc_write, 0, 0);
        shader->desc_updated[image_idx] = true;
    }

    vkCmdBindDescriptorSets(combuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            shader->pipeline.pipe_layout, 0, 1, &global_desc, 0,
                            0);
}

void vk_material_shader_set_model(vulkan_context_t         *ctx,
                                  vulkan_material_shader_t *shader,
                                  mat4                      model) {
    if (ctx && shader) {
        uint32_t        image_idx = ctx->image_idx;
        VkCommandBuffer combuff   = ctx->graphic_comm_buffer[image_idx].handle;

        vkCmdPushConstants(combuff, shader->pipeline.pipe_layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &model);
    }
}

void vk_material_shader_apply(vulkan_context_t         *ctx,
                              vulkan_material_shader_t *shader,
                              material_t               *material) {
    if (ctx && shader) {
        uint32_t        image_idx = ctx->image_idx;
        VkCommandBuffer combuff   = ctx->graphic_comm_buffer[image_idx].handle;

        vulkan_material_shader_state_t *obj_state =
            &shader->instance_states[material->internal_id];
        VkDescriptorSet      obj_desc_set = obj_state->desc_sets[image_idx];

        VkWriteDescriptorSet desc_writes[VULKAN_MATERIAL_SHADER_DESC_COUNT];
        memory_zero(desc_writes, sizeof(VkWriteDescriptorSet) *
                                     VULKAN_MATERIAL_SHADER_DESC_COUNT);
        uint32_t desc_count = 0;
        uint32_t desc_idx   = 0;

        // desc 0 - uniform buffer
        uint32_t range  = sizeof(vulkan_material_shader_instance_ubo_t);
        uint64_t offset = sizeof(vulkan_material_shader_instance_ubo_t) * material->internal_id;
        vulkan_material_shader_instance_ubo_t obo;
        obo.diffuse_color = material->diffuse_color;

        // load data to buffer
        vk_buffer_load_data(ctx, &shader->obj_uni_buffer, offset, range, 0,
                            &obo);

        uint32_t *global_ubo_gen =
            &obj_state->desc_states[desc_idx].gen[image_idx];
        if (*global_ubo_gen == INVALID_ID || *global_ubo_gen != material->gen) {
            VkDescriptorBufferInfo buffer_info;
            buffer_info.buffer       = shader->obj_uni_buffer.handle;
            buffer_info.offset       = offset;
            buffer_info.range        = range;

            VkWriteDescriptorSet des = {};
            des.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            des.dstSet               = obj_desc_set;
            des.dstBinding           = desc_idx;
            des.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            des.descriptorCount      = 1;
            des.pBufferInfo          = &buffer_info;

            desc_writes[desc_count]  = des;
            desc_count++;

            *global_ubo_gen = material->gen;
        }
        desc_idx++;

        // Samplers
        const uint32_t        sampler_count = 1;
        VkDescriptorImageInfo img_info[1];
        for (uint32_t sampler_idx = 0; sampler_idx < sampler_count;
             ++sampler_idx) {
            texture_use_t use = shader->sampler_uses[sampler_idx];
            texture_t    *tt  = 0;
            switch (use) {
            case TEXTURE_USE_MAP_DIFFUSE:
                tt = material->diffuse_map.texture;
                break;
            default:
                ar_FATAL("Unable to bind sampler to unknown use.");
                return;
            }

            uint32_t *desc_gen =
                &obj_state->desc_states[desc_idx].gen[image_idx];
            uint32_t *desc_id = &obj_state->desc_states[desc_idx].id[image_idx];

            if (tt->gen == INVALID_ID) {
                tt        = texture_sys_get_default_tex();
                *desc_gen = INVALID_ID;
            }

            if (tt && (*desc_id != tt->id || *desc_gen != tt->gen ||
                       *desc_gen == INVALID_ID)) {
                vulkan_texture_data_t *internal_data =
                    (vulkan_texture_data_t *)tt->internal_data;

                // Assign View and Samplers.
                img_info[sampler_idx].imageLayout =
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                img_info[sampler_idx].imageView =
                    internal_data->image.image_view;
                img_info[sampler_idx].sampler = internal_data->sampler;

                VkWriteDescriptorSet descript = {};
                descript.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descript.dstSet     = obj_desc_set;
                descript.dstBinding = desc_idx;
                descript.descriptorType =
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descript.descriptorCount = 1;
                descript.pImageInfo      = &img_info[sampler_idx];

                desc_writes[desc_count]  = descript;
                desc_count++;

                // Sync frame gen if not using default texture.
                if (tt->gen != INVALID_ID) {
                    *desc_gen = tt->gen;
                    *desc_id  = tt->id;
                }
                desc_idx++;
            }
        }

        if (desc_count > 0) {
            vkUpdateDescriptorSets(ctx->device.logic_dev, desc_count,
                                   desc_writes, 0, 0);
        }

        vkCmdBindDescriptorSets(combuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                shader->pipeline.pipe_layout, 1, 1,
                                &obj_desc_set, 0, 0);
    }
}

b8 vk_material_shader_acquire_rsc(vulkan_context_t         *ctx,
                                  vulkan_material_shader_t *shader,
                                  material_t               *material) {
    material->internal_id = shader->obj_uniform_buffer_idx;
    shader->obj_uniform_buffer_idx++;

    vulkan_material_shader_state_t *obj_state =
        &shader->instance_states[material->internal_id];
    for (uint32_t i = 0; i < VULKAN_MATERIAL_SHADER_DESC_COUNT; ++i) {
        for (uint32_t j = 0; j < 4; ++j) {
            obj_state->desc_states[i].gen[j] = INVALID_ID;
            obj_state->desc_states[i].id[j]  = INVALID_ID;
        }
    }

    VkDescriptorSetLayout layouts[4]       = {shader->obj_desc_set_layout,
                                              shader->obj_desc_set_layout,
                                              shader->obj_desc_set_layout,
                                              shader->obj_desc_set_layout};

    // allocate descriptor sets
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = shader->obj_desc_pool;
    alloc_info.descriptorSetCount = 4;
    alloc_info.pSetLayouts        = layouts;

    VkResult result =
        vkAllocateDescriptorSets(ctx->device.logic_dev, &alloc_info,
                                 obj_state->desc_sets);
    if (result != VK_SUCCESS) {
        ar_ERROR("Error allocating descriptor sets in shader.");
        return false;
    }

    return true;
}

void vk_material_shader_release_rsc(vulkan_context_t         *ctx,
                                    vulkan_material_shader_t *shader,
                                    material_t               *material) {
    vulkan_material_shader_state_t *ints_states =
        &shader->instance_states[material->internal_id];
    const uint32_t desc_set_count = 4;

    /* wait for pending operations */
    vkDeviceWaitIdle(ctx->device.logic_dev);

    VkResult       result =
        vkFreeDescriptorSets(ctx->device.logic_dev, shader->obj_desc_pool,
                             desc_set_count, ints_states->desc_sets);
    if (result != VK_SUCCESS) {
        ar_ERROR("Error free object shader descriptor sets");
    }

    for (uint32_t i = 0; i < VULKAN_MATERIAL_SHADER_DESC_COUNT; ++i) {
        for (uint32_t j = 0; j < 4; ++j) {
            ints_states->desc_states[i].gen[j] = INVALID_ID;
            ints_states->desc_states[i].id[j]  = INVALID_ID;
        }
    }

    material->internal_id = INVALID_ID;
}
