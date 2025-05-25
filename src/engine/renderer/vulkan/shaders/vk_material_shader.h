#ifndef __VULKAN_OBJECT_SHADER_H__
#define __VULKAN_OBJECT_SHADER_H__

#include "engine/renderer/renderer_type.h"
#include "engine/renderer/vulkan/vk_type.h"

b8 vk_material_shader_init(vulkan_context_t         *ctx,
                           vulkan_material_shader_t *shader);

void vk_material_shader_shut(vulkan_context_t *ctx, vulkan_material_shader_t *shader);
void vk_material_shader_use(vulkan_context_t *ctx, vulkan_material_shader_t *shader);

void vk_material_shader_update_global_state(vulkan_context_t       *ctx,
                                       vulkan_material_shader_t *shader,
                                       float                   delta_time);

void vk_material_shader_set_model(vulkan_context_t         *ctx,
                                  vulkan_material_shader_t *shader, mat4 model);

void vk_material_shader_apply(vulkan_context_t         *ctx,
                              vulkan_material_shader_t *shader,
                              material_t               *material);

b8   vk_material_shader_acquire_rsc(vulkan_context_t         *ctx,
                                    vulkan_material_shader_t *shader,
                                    material_t               *material);

void vk_material_shader_release_rsc(vulkan_context_t         *ctx,
                                    vulkan_material_shader_t *shader,
                                    material_t               *material);

#endif //__VULKAN_OBJECT_SHADER_H__
