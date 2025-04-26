#ifndef __VULKAN_PIPELINE_H__
#define __VULKAN_PIPELINE_H__

#include "engine/renderer/vulkan/vk_type.h"

b8 vk_pipeline_init(vulkan_context_t *ctx, vulkan_renderpass_t *renderpass,
                    uint32_t                           attr_count,
                    VkVertexInputAttributeDescription *attr,
                    uint32_t                           set_layout_count,
                    VkDescriptorSetLayout *set_layout, uint32_t stg_count,
                    VkPipelineShaderStageCreateInfo *stg, VkViewport viewport,
                    VkRect2D scissor, b8 is_wireframe, vulkan_pipeline_t *pipeline);

void vk_pipeline_shut(vulkan_context_t *ctx, vulkan_pipeline_t *pipeline);
void vk_pipeline_bind(vulkan_commandbuffer_t *combuff,
                      VkPipelineBindPoint     bind_point,
                      vulkan_pipeline_t      *pipeline);

#endif //__VULKAN_PIPELINE_H__
