#include "engine/resources/shader_loader.h"

#include "engine/container/dyn_array.h"
#include "engine/core/ar_strings.h"
#include "engine/core/logger.h"
#include "engine/math/maths.h"
#include "engine/memory/memory.h"
#include "engine/platform/filesystem.h"
#include "engine/resources/resc_type.h"
#include "engine/resources/loader_utils.h"
#include "engine/systems/resource_sys.h"

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
b8 shader_loader_load(resource_loader_t *self, const char *name,
                      resource_t *resc) {
    if (!self || !name || !resc) {
        return false;
    }

    char *format_str = "%s/%s/%s%s";
	char full_path[512];
    string_format(full_path, format_str, resource_sys_base_path(),
                  self->type_path, name, ".shadercfg");

	file_handle_t f;
	if (!filesystem_open(full_path, MODE_READ, false, &f)) {
		ar_ERROR("shader_loader_load - unable to open file: '%s'", full_path);
		return false;
	}

	resc->full_path = string_duplicate(full_path);

    shader_config_t *resc_data =
        memory_alloc(sizeof(shader_config_t), MEMTAG_RESOURCE);
    resc_data->attr_count      = 0;
    resc_data->attributes = dyn_array_create(shader_attr_config_t);
	resc_data->uniform_count = 0;
	resc_data->uniforms = dyn_array_create(shader_uniform_config_t);
	resc_data->stage_count = 0;
	resc_data->stages = dyn_array_create(shader_stage_t);
	resc_data->use_instances = false;
	resc_data->use_locals = false;
	resc_data->stage_count = 0;
	resc_data->stage_names = dyn_array_create(char *);
	resc_data->stage_filenames = dyn_array_create(char *);
	resc_data->renderpass_name = 0;
	resc_data->name = 0;

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
			resc_data->name = string_duplicate(trim_value);
		} else if (string_equali(trim_name, "renderpass")) {
			resc_data->renderpass_name = string_duplicate(trim_value);
        } else if (string_equali(trim_name, "stages")) {

			/* Parse stages */
			char **sn = dyn_array_create(char *);
			uint32_t count = string_split(trim_value, ',', &sn, true, true);
			resc_data->stage_names = sn;

			// Ensure stage name & stage file name count are the same, as they
			// should align
			if (resc_data->stage_count == 0){
				resc_data->stage_count = count;
			} else if (resc_data->stage_count != count) {
                ar_ERROR("shader_loader_load - Invalid file layout. Mismatch "
                         "count.");
            }

			/* Parse each stage & add right type to array */
            for (uint8_t i = 0; i < resc_data->stage_count; ++i) {
                if (string_equali(sn[i], "frag") ||
                    string_equali(sn[i], "fragment")) {
                    dyn_array_push(resc_data->stages, SHADER_STAGE_FRAGMENT);
                } else if (string_equali(sn[i], "vert") ||
                           string_equali(sn[i], "vertex")) {
                    dyn_array_push(resc_data->stages, SHADER_STAGE_VERTEX);
                } else if (string_equali(sn[i], "geom") ||
                           string_equali(sn[i], "geometry")) {
                    dyn_array_push(resc_data->stages, SHADER_STAGE_GEOMETRY);
                } else if (string_equali(sn[i], "comp") ||
                           string_equali(sn[i], "compute")) {
                    dyn_array_push(resc_data->stages, SHADER_STAGE_COMPUTE);
                } else {
                    ar_ERROR("shader_loader_load - Invalid file layout. "
                             "Unrecognized '%s'",
                             sn[i]);
                }
            }

        } else if (string_equali(trim_name, "stagefiles")) {
			/* Parse stage file names */
			resc_data->stage_filenames = dyn_array_create(char *);
            uint32_t count =
                string_split(trim_value, ',', &resc_data->stage_filenames, true,
                             true);
            if (resc_data->stage_count == 0) {
                resc_data->stage_count = count;
            } else if (resc_data->stage_count != count) {
                ar_ERROR("shader_loader_load - Invalid file layout. Count "
                         "mismatch.");
            }

        } else if (string_equali(trim_name, "use_instance")) {
			string_to_bool(trim_value, &resc_data->use_instances);
		} else if (string_equali(trim_name, "use_local")) {
			string_to_bool(trim_value, &resc_data->use_locals);
		} else if (string_equali(trim_name, "attribute")) {
			
			/* Parse Attributes */
			char **fields = dyn_array_create(char *);
            uint32_t field_count =
                string_split(trim_value, ',', &fields, true, true);
            if (field_count != 2) {
                ar_ERROR("shader_loader_load - Invalid file layout. Attribute "
                         "fields must be 'type, name'. Skip.");
            } else {
                shader_attr_config_t attr;
				/* Parse field types */
				if (string_equali(fields[0], "float")) {
					attr.type = SHADER_ATTR_FLOAT32;
					attr.size = 4;
				} else if (string_equali(fields[0], "vec2")) {
					attr.type = SHADER_ATTR_FLOAT32_2;
					attr.size = 8;
				} else if (string_equali(fields[0], "vec3")) {
					attr.type = SHADER_ATTR_FLOAT32_3;
					attr.size = 12;
				} else if (string_equali(fields[0], "vec4")) {
					attr.type = SHADER_ATTR_FLOAT32_4;
					attr.size = 16;
				} else if (string_equali(fields[0], "uint8_t")) {
					attr.type = SHADER_ATTR_UINT8;
					attr.size = 1;
				} else if (string_equali(fields[0], "uint16_t")) {
					attr.type = SHADER_ATTR_UINT16;
					attr.size = 2;
				} else if (string_equali(fields[0], "uint32_t")) {
					attr.type = SHADER_ATTR_UINT32;
					attr.size = 4;
				} else if (string_equali(fields[0], "int8_t")) {
					attr.type = SHADER_ATTR_INT8;
					attr.size = 1;
				} else if (string_equali(fields[0], "int16_t")) {
					attr.type = SHADER_ATTR_INT16;
					attr.size = 2;
				} else if (string_equali(fields[0], "int32_t")) {
					attr.type = SHADER_ATTR_INT32;
					attr.size = 4;
				} else {
                    ar_ERROR(
                        "shader_loader_load - Invalid file layout. Attribute "
                        "type must be float, vec2, vec3, vec4, int8_t, "
                        "int16_t, int32_t, uint8_t, uint16_t, uint32_t");
                    ar_WARNING("Defaulting to float");
                    attr.type = SHADER_ATTR_FLOAT32;
					attr.size = 4;
				}

				attr.name_length = string_length(fields[1]);
				attr.name = string_duplicate(fields[1]);
				dyn_array_push(resc_data->attributes, attr);
				resc_data->attr_count++;
            }

			string_clean_split_array(fields);
			dyn_array_destroy(fields);
        } else if (string_equali(trim_name, "uniform")) {

			/* Parse Uniform */
			char **fields = dyn_array_create(char *);
            uint32_t field_count =
                string_split(trim_value, ',', &fields, true, true);
            if (field_count != 3) {
                ar_ERROR("shader_loader_load - Invalid file layout. Uniform "
                         "must be 'type, scope, name'. Skip.");
            } else {
                shader_uniform_config_t uniform;
				/* Parse field types */
				if (string_equali(fields[0], "float")) {
					uniform.type = SHADER_UNIFORM_FLOAT32;
					uniform.size = 4;
				} else if (string_equali(fields[0], "vec2")) {
					uniform.type = SHADER_UNIFORM_FLOAT32_2;
					uniform.size = 8;
				} else if (string_equali(fields[0], "vec3")) {
					uniform.type = SHADER_UNIFORM_FLOAT32_3;
					uniform.size = 12;
				} else if (string_equali(fields[0], "vec4")) {
					uniform.type = SHADER_UNIFORM_FLOAT32_4;
					uniform.size = 16;
				} else if (string_equali(fields[0], "uint8_t")) {
					uniform.type = SHADER_UNIFORM_UINT8;
					uniform.size = 1;
				} else if (string_equali(fields[0], "uint16_t")) {
					uniform.type = SHADER_UNIFORM_UINT16;
					uniform.size = 2;
				} else if (string_equali(fields[0], "uint32_t")) {
					uniform.type = SHADER_UNIFORM_UINT32;
					uniform.size = 4;
				} else if (string_equali(fields[0], "int8_t")) {
					uniform.type = SHADER_UNIFORM_INT8;
					uniform.size = 1;
				} else if (string_equali(fields[0], "int16_t")) {
					uniform.type = SHADER_UNIFORM_INT16;
					uniform.size = 2;
				} else if (string_equali(fields[0], "int32_t")) {
					uniform.type = SHADER_UNIFORM_INT32;
					uniform.size = 4;
				} else if (string_equali(fields[0], "mat4")) {
					uniform.type = SHADER_UNIFORM_MATRIX_4;
					uniform.size = 64;
                } else if (string_equali(fields[0], "samp") ||
                           string_equali(fields[0], "sampler")) {
                    uniform.type = SHADER_UNIFORM_SAMPLER;
                    uniform.size = 0;
                } else {
                    ar_ERROR(
                        "shader_loader_load - Invalid file layout. Uniforms"
                        "type must be float, vec2, vec3, vec4, int8_t, "
                        "int16_t, int32_t, uint8_t, uint16_t, uint32_t or "
                        "mat4");
                    ar_WARNING("Defaulting to float");
                    uniform.type = SHADER_UNIFORM_FLOAT32;
					uniform.size = 4;
                }

				/* Parse scope */
                if (string_equal(fields[1], "0")) {
					uniform.scope = SHADER_SCOPE_GLOBAL;
				} else if (string_equal(fields[1], "1")) {
					uniform.scope = SHADER_SCOPE_INSTANCE;
				} else if (string_equal(fields[1], "2")) {
					uniform.scope = SHADER_SCOPE_LOCAL;
				} else {
                    ar_ERROR(
                        "shader_loader_load - Invalid file layout. Uniform "
                        "scope must be 0=global, 1=instance, 2=local.");
                    ar_WARNING("Defaulting to global");
                    uniform.scope = SHADER_SCOPE_GLOBAL;
				}

				uniform.name_length = string_length(fields[2]);
				uniform.name = string_duplicate(fields[2]);
				dyn_array_push(resc_data->uniforms, uniform);
				resc_data->uniform_count++;
            }
            string_clean_split_array(fields);
			dyn_array_destroy(fields);
		}

		// TODO: More fields
		
		memory_zero(line_buff, sizeof(char) * 512);
		line_number++;
	}
	
	filesystem_close(&f);

	resc->data = resc_data;
	resc->data_size = sizeof(shader_config_t);

	return true;
}

void shader_loader_unload(resource_loader_t *self, resource_t *resc) {
	shader_config_t *data = (shader_config_t *)resc->data;

	string_clean_split_array(data->stage_filenames);
	dyn_array_destroy(data->stage_filenames);

	string_clean_split_array(data->stage_names);
	dyn_array_destroy(data->stage_names);

	dyn_array_destroy(data->stages);

	/* Clean Attribute */
	uint32_t count = dyn_array_length(data->attributes);
	for (uint32_t i = 0; i < count; ++i) {
		uint32_t len = string_length(data->attributes[i].name);
        memory_free(data->attributes[i].name, sizeof(char) * (len + 1),
                    MEMTAG_STRING);
    }
    dyn_array_destroy(data->attributes);

	/* Clean Uniforms */
	count = dyn_array_length(data->uniforms);
	for (uint32_t i = 0; i < count; ++i) {
		uint32_t len = string_length(data->uniforms[1].name);
        memory_free(data->uniforms[i].name, sizeof(char) * (len + 1),
                    MEMTAG_STRING);
    }
    dyn_array_destroy(data->uniforms);

    memory_free(data->renderpass_name,
                sizeof(char) * (string_length(data->renderpass_name) + 1),
                MEMTAG_STRING);
    memory_free(data->name, sizeof(char) * (string_length(data->name) + 1),
                MEMTAG_STRING);
    memory_zero(data, sizeof(shader_config_t));

	if (!resc_unload(self, resc, MEMTAG_RESOURCE)) {
		ar_WARNING("shader_loader_unload - called with nullptr");
	}
}

/* ========================================================================== */
/* ========================================================================== */
resource_loader_t loader_shader_rsc_init(void) {
	resource_loader_t loader;
	loader.type = RESC_TYPE_SHADER;
	loader.custom_type = 0;
	loader.load = shader_loader_load;
	loader.unload = shader_loader_unload;
	loader.type_path = "shaders";

	return loader;
}

