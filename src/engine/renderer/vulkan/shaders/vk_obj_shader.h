#ifndef __VULKAN_OBJECT_SHADER_H__
#define __VULKAN_OBJECT_SHADER_H__

#include "engine/renderer/renderer_type.h"
#include "engine/renderer/vulkan/vk_type.h"

b8   vk_obj_shader_init(vulkan_context_t *ctx, vulkan_object_shader_t *shader);
void vk_obj_shader_shut(vulkan_context_t *ctx, vulkan_object_shader_t *shader);
void vk_obj_shader_use(vulkan_context_t *ctx, vulkan_object_shader_t *shader);

void vk_obj_shader_update_global_state(vulkan_context_t       *ctx,
                                       vulkan_object_shader_t *shader);

void vk_obj_shader_update_obj(vulkan_context_t       *ctx,
                              vulkan_object_shader_t *shader, mat4 model);

b8   vk_obj_shader_acquire_rsc(vulkan_context_t       *ctx,
                               vulkan_object_shader_t *shader, uint32_t *obj_id);
void vk_obj_shader_release_rsc(vulkan_context_t       *ctx,
                               vulkan_object_shader_t *shader,
                               uint32_t               *obj_id);

#endif //__VULKAN_OBJECT_SHADER_H__
