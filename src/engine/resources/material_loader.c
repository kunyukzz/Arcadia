#include "engine/resources/material_loader.h"

#include "engine/core/ar_strings.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"
#include "engine/math/maths.h"
#include "engine/platform/filesystem.h"
#include "engine/resources/resc_type.h"
#include "engine/resources/loader_utils.h"
#include "engine/systems/resource_sys.h"

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
b8 material_loader_load(resource_loader_t *self, const char *name,
                        resource_t *resc) {
    if (!self || !name || !resc) return false;

    char *format_str = "%s/%s/%s%s";
	char full_path[512];
    string_format(full_path, format_str, resource_sys_base_path(),
                  self->type_path, name, ".ar_mat");

	file_handle_t f;
	if (!filesystem_open(full_path, MODE_READ, false, &f)) {
		ar_ERROR("material_loader_load - unable to open file: '%s'", full_path);
		return false;
	}

    // TODO: Should use allocator.
	resc->full_path = string_duplicate(full_path);

	// TODO: Should use allocator.
    material_config_t *resc_data =
        memory_alloc(sizeof(material_config_t), MEMTAG_MATERIAL);
	resc_data->shader_name = "Builtin.Material";
	resc_data->auto_release = true;
	resc_data->diffuse_color = vec4_one();
	resc_data->diffuse_map_name[0] = 0;
	string_ncopy(resc_data->name, name, MATERIAL_NAME_MAX_LENGTH);

	// Read each line.
	char line_buff[512] = "";
	char *p = &line_buff[0];
	uint64_t line_length = 0;
	uint32_t line_number = 1;

	while (filesystem_read_line(&f, 511, &p, &line_length)) {
		char *trim = string_trim(line_buff);
		line_length = string_length(trim);
		if (line_length < 1 || trim[0] == '#') {
			line_number++;
			continue;
		}

		// Split var/value
		int32_t equal_idx = string_index_of(trim, '=');
		if (equal_idx == -1) {
            ar_WARNING("Format issue on: '%s'->'=' token not found. Skip line",
                       full_path, line_number);
            line_number++;
            continue;
		}

		char raw_name[64];
		memory_zero(raw_name, sizeof(char) * 64);
		string_mid(raw_name, trim, 0, equal_idx);
		char *trim_name = string_trim(raw_name);

		char raw_value[446];
		memory_zero(raw_value, sizeof(char) * 446);
		string_mid(raw_value, trim, equal_idx + 1, -1);
		char *trim_value = string_trim(raw_value);

		// process variable
		if (string_equali(trim_name, "version")) {
			// TODO: Version
		} else if (string_equali(trim_name, "name")) {
			string_ncopy(resc_data->name, trim_value, MATERIAL_NAME_MAX_LENGTH);
		} else if (string_equali(trim_name, "diffuse_map_name")) {
            string_ncopy(resc_data->diffuse_map_name, trim_value,
                         TEXTURE_NAME_MAX_LENGTH);
        } else if (string_equali(trim_name, "diffuse_color")) {
            // parse Color
			if (!string_to_vec4(trim_value, &resc_data->diffuse_color)) {
				ar_WARNING("Error parsing diffuse_color in file: '%s'", full_path);
				resc_data->diffuse_color = vec4_one(); // set white
			}
		} else if (string_equali(trim_name, "shader")) {
			resc_data->shader_name = string_duplicate(trim_value);
        } else {
            ar_WARNING("Unknown field '%s' in material file: '%s'",
                       trim_name, full_path);
        }

		// TODO: More fields
		memory_zero(line_buff, sizeof(char) * 512);
		line_number++;
	}

	filesystem_close(&f);
	resc->data = resc_data;
	resc->data_size = sizeof(material_config_t);
	resc->name = name;

    return true;
}

void material_loader_unload(resource_loader_t *self, resource_t *resc) {
	if (!resc_unload(self, resc, MEMTAG_MATERIAL)) {
		ar_WARNING("material_loader_unload - call with nullptr");
	}
}
/* ========================================================================== */
/* ========================================================================== */

resource_loader_t loader_material_rsc_init(void) {
	resource_loader_t loader;
	loader.type = RESC_TYPE_MATERIAL;
	loader.custom_type = 0;
	loader.load = material_loader_load;
	loader.unload = material_loader_unload;
	loader.type_path = "materials";

	return loader;
}

