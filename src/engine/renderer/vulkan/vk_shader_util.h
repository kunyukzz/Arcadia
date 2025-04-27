#ifndef __VULKAN_SHADER_UTIL_H__
#define __VULKAN_SHADER_UTIL_H__

#include "engine/renderer/vulkan/vk_type.h"
#include "engine/core/logger.h"
#include "engine/core/strings.h"
#include "engine/memory/memory.h"
#include "engine/platform/filesystem.h"

#define SHADER_DIR "assets/shaders/"

_arinline b8 vk_shader_module_init(vulkan_context_t *ctx, const char *name,
                         const char           *type_str,
                         VkShaderStageFlagBits shader_stg_flag,
                         uint32_t stg_idx, vulkan_shader_stage_t *shader_stg) {
	char filename[512];
	string_format(filename, SHADER_DIR "%s.%s.spv", name, type_str);

	memory_zero(&shader_stg[stg_idx].cr_info, sizeof(VkShaderModuleCreateInfo));
    shader_stg[stg_idx].cr_info.sType =
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    file_handle_t handle;
	if (!filesystem_open(filename, MODE_READ, true,  &handle)) {
		ar_ERROR("Unable to read shader module: '%s'", filename);
		return false;
	}

	uint64_t size = 0;
	uint8_t *file_buff = 0;
	if (!filesystem_read_all_byte(&handle, &file_buff, &size)) {
		ar_ERROR("Unable to read binary shader module: '%s'", filename);
		return false;
	}
	shader_stg[stg_idx].cr_info.codeSize = size;

	// file_buff is 16-byte aligned from memory allocator, safe to cast
	shader_stg[stg_idx].cr_info.pCode = (uint32_t *)(void *)file_buff;

	filesystem_close(&handle);

    VK_CHECK(vkCreateShaderModule(ctx->device.logic_dev,
                                  &shader_stg[stg_idx].cr_info, ctx->alloc,
                                  &shader_stg[stg_idx].handle));

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
