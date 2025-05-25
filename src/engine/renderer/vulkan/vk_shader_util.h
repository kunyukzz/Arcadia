#ifndef __VULKAN_SHADER_UTIL_H__
#define __VULKAN_SHADER_UTIL_H__

#include "engine/renderer/vulkan/vk_type.h"
#include "engine/core/logger.h"
#include "engine/core/ar_strings.h"
#include "engine/memory/memory.h"
#include "engine/systems/resource_sys.h"

_arinline b8 vk_shader_module_init(vulkan_context_t *ctx, const char *name,
                         const char           *type_str,
                         VkShaderStageFlagBits shader_stg_flag,
                         uint32_t stg_idx, vulkan_shader_stage_t *shader_stg) {
	char filename[512];
	string_format(filename, "shaders/%s.%s.spv", name, type_str);
    //ar_TRACE("File name: %s", filename);

	resource_t binary_resc;
	if (!resource_sys_load(filename, RESC_TYPE_BINARY, &binary_resc)) {
		ar_ERROR("unable to read shader module: %s", filename);
		return false;
	}

	memory_zero(&shader_stg[stg_idx].cr_info, sizeof(VkShaderModuleCreateInfo));
    shader_stg[stg_idx].cr_info.sType =
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_stg[stg_idx].cr_info.codeSize = binary_resc.data_size;
    shader_stg[stg_idx].cr_info.pCode = (uint32_t *)binary_resc.data;

    VK_CHECK(vkCreateShaderModule(ctx->device.logic_dev,
                                  &shader_stg[stg_idx].cr_info, ctx->alloc,
                                  &shader_stg[stg_idx].handle));

	resource_sys_unload(&binary_resc);

    memory_zero(&shader_stg[stg_idx].shader_stg_cr_info,
                sizeof(VkPipelineShaderStageCreateInfo));
    shader_stg[stg_idx].shader_stg_cr_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stg[stg_idx].shader_stg_cr_info.stage  = shader_stg_flag;
    shader_stg[stg_idx].shader_stg_cr_info.module = shader_stg[stg_idx].handle;
	shader_stg[stg_idx].shader_stg_cr_info.pName = "main";

	return true;
}

#endif //__VULKAN_SHADER_UTIL_H__
