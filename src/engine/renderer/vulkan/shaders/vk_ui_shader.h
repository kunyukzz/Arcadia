#ifndef __VULKAN_UI_SHADER_H__
#define __VULKAN_UI_SHADER_H__

#include "engine/renderer/vulkan/vk_type.h"
#include "engine/renderer/renderer_type.h"

b8   vk_ui_shader_init(vulkan_context_t *ctx, vulkan_ui_shader_t *shader);
void vk_ui_shader_shut(vulkan_context_t *ctx, vulkan_ui_shader_t *shader);

void vk_ui_shader_use(vulkan_context_t *ctx, vulkan_ui_shader_t *shader);
void vk_ui_shader_update_global(vulkan_context_t   *ctx,
                                vulkan_ui_shader_t *shader, float delta_time);

void vk_ui_shader_set_model(vulkan_context_t *ctx, vulkan_ui_shader_t *shader,
                            mat4 model);
void vk_ui_shader_apply_material(vulkan_context_t   *ctx,
                                 vulkan_ui_shader_t *shader,
                                 material_t         *material);

b8 vk_ui_shader_acquire_resc(vulkan_context_t *ctx, vulkan_ui_shader_t *shader,
                             material_t *material);
void vk_ui_shader_release_resc(vulkan_context_t   *ctx,
                               vulkan_ui_shader_t *shader,
                               material_t         *material);

#endif //__VULKAN_UI_SHADER_H__
