#include "engine/systems/material_sys.h"

#include "engine/container/hashtable.h"
#include "engine/core/logger.h"
#include "engine/core/ar_strings.h"
#include "engine/math/maths.h"
#include "engine/renderer/renderer_fe.h"
#include "engine/systems/texture_sys.h"
#include "engine/systems/resource_sys.h"
#include "engine/systems/shader_sys.h"

typedef struct material_shader_uni_loc_t {
	uint16_t projection;
	uint16_t view;
	uint16_t diffuse_color;
	uint16_t diffuse_texture;
	uint16_t model;
} material_shader_uni_loc_t;

typedef struct ui_shader_uni_loc_t {
	uint16_t projection;
	uint16_t view;
	uint16_t diffuse_color;
	uint16_t diffuse_texture;
	uint16_t model;
} ui_shader_uni_loc_t;

typedef struct material_sys_state_t {
	material_sys_config_t config;
	material_t default_material;
	material_t *reg_materials;
	hashtable_t reg_material_table;
	material_shader_uni_loc_t material_loc;
	ui_shader_uni_loc_t ui_loc;
	uint32_t material_shader_id;
	uint32_t ui_shader_id;
} material_sys_state_t;

typedef struct material_ref_t {
	uint64_t ref_count;
	uint32_t handle;
	b8 auto_release;
} material_ref_t;

static material_sys_state_t *p_state = 0;

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
b8 default_material_init(material_sys_state_t *state) {
	memory_zero(&state->default_material, sizeof(material_t));

	state->default_material.id = INVALID_ID;
	state->default_material.gen = INVALID_ID;

    string_ncopy(state->default_material.name, DEFAULT_MATERIAL_NAME,
                 MATERIAL_NAME_MAX_LENGTH);

    state->default_material.diffuse_color = vec4_one();
	state->default_material.diffuse_map.used = TEXTURE_USE_MAP_DIFFUSE;
	state->default_material.diffuse_map.texture = texture_sys_get_default_tex();

	// TODO: create renderer material initialized

    shader_t *s = shader_sys_get(BUILTIN_SHADER_NAME_MATERIAL);
    if (!renderer_shader_acquire_inst_resc(s, &state->default_material
                                                   .internal_id)) {
        ar_FATAL("Failed to acquire renderer resources for default material. "
                 "Application cannot continue.");
        return false;
    }

    return true;
}

b8 load_material(material_config_t config, material_t *mt) {
	memory_zero(mt, sizeof(material_t));

	string_ncopy(mt->name, config.name, MATERIAL_NAME_MAX_LENGTH);

	/* Type */
	mt->shader_id = shader_sys_get_id(config.shader_name);

	/* Diffuse Color */
	mt->diffuse_color = config.diffuse_color;

	/* Diffuse Map */
	if (string_length(config.diffuse_map_name) > 0) {
		mt->diffuse_map.used = TEXTURE_USE_MAP_DIFFUSE;
        mt->diffuse_map.texture =
            texture_sys_acquire(config.diffuse_map_name, true);

		if (!mt->diffuse_map.texture) {
            ar_WARNING("Unable to load texture '%s', using default material.",
                       config.diffuse_map_name, mt->name);
            mt->diffuse_map.texture = texture_sys_get_default_tex();
        }
    } else {
		mt->diffuse_map.used = TEXTURE_USE_UNKNOWN;
		mt->diffuse_map.texture = 0;
	}

	// TODO: Other maps

	shader_t *s = shader_sys_get(config.shader_name);
	if (!s) {
        ar_ERROR("Unable to load material because its shader was not found: "
                 "'%s'. Likely problem with material asset",
                 config.shader_name);
        return false;
    }

	if (!renderer_shader_acquire_inst_resc(s, &mt->internal_id)) {
        ar_ERROR("Failed to acquire renderer resources for material '%s'",
                 mt->name);
        return false;
    }

	return true;
}

void default_material_shut(material_t *mt) {
	ar_TRACE("Kill Material '%s'...", mt->name);

	if (mt->diffuse_map.texture) {
		texture_sys_release(mt->diffuse_map.texture->name);
	}

	if (mt->shader_id != INVALID_ID && mt->internal_id != INVALID_ID) {
		renderer_shader_release_inst_resc(shader_sys_get_by_id(mt->shader_id), mt->internal_id);
		mt->shader_id = INVALID_ID;
	}
	
	memory_zero(mt, sizeof(material_t));
	mt->id = INVALID_ID;
	mt->gen = INVALID_ID;
	mt->internal_id = INVALID_ID;
}
/* ========================================================================== */
/* ========================================================================== */

b8   material_sys_init(uint64_t *memory_require, void *state,
                       material_sys_config_t config) {
	if (config.max_material_count == 0) {
		ar_FATAL("material_sys_init - max material count must be > 0");
		return false;
	}

	uint64_t struct_req = sizeof(material_sys_state_t);
	uint64_t array_req = sizeof(material_t) * config.max_material_count;
	uint64_t table_req = sizeof(material_ref_t) * config.max_material_count;
	*memory_require = struct_req + array_req + table_req;

	if (!state) {
		return true;
	}

	p_state = state;
	p_state->config = config;
	p_state->material_shader_id = INVALID_ID;
	p_state->material_loc.diffuse_color = INVALID_ID_U16;
	p_state->material_loc.diffuse_texture = INVALID_ID_U16;
	p_state->ui_shader_id = INVALID_ID;
	p_state->ui_loc.diffuse_color = INVALID_ID_U16;
	p_state->ui_loc.diffuse_texture = INVALID_ID_U16;

	void *array_block = (char *)state + struct_req;
	p_state->reg_materials = array_block;

	void *table_block = (char *)array_block + table_req;
    hashtable_init(sizeof(material_ref_t), config.max_material_count,
                   table_block, false, &p_state->reg_material_table);

	// fill hashtable 
	material_ref_t invalid_ref;
	invalid_ref.auto_release = false;
	invalid_ref.handle = INVALID_ID;
	invalid_ref.ref_count = 0;
	hashtable_fill(&p_state->reg_material_table, &invalid_ref);

	// invalidate all material in array
	uint32_t count = p_state->config.max_material_count;
	for (uint32_t i = 0; i < count; ++i) {
		p_state->reg_materials[i].id = INVALID_ID;
		p_state->reg_materials[i].gen = INVALID_ID;
		p_state->reg_materials[i].internal_id = INVALID_ID;
	}

	if (!default_material_init(p_state)) {
		ar_FATAL("Failed to create default material. Application Stop");
		return false;
	}

	return true;
}

void material_sys_shut(void *state) {
	material_sys_state_t *s = (material_sys_state_t *)state;

	if (s) {
		uint32_t count = s->config.max_material_count;
		for (uint32_t i = 0; i < count; ++i) {
			if (s->reg_materials[i].id != INVALID_ID) {
				default_material_shut(&s->reg_materials[i]);
			}
		}

		default_material_shut(&s->default_material);
	}

	p_state = 0;
}

material_t *material_sys_get_default(void) {
	if (p_state) {
		return &p_state->default_material;
	}

    ar_FATAL("material_sys_get_default - called before system initialized");
    return 0;
}

material_t *material_sys_acquire(const char *name) {
    resource_t material_resc;
    if (!resource_sys_load(name, RESC_TYPE_MATERIAL, &material_resc)) {
        ar_ERROR("Failed to load material.");
        return 0;
    }

    material_t *m;
    m = material_sys_acquire_from_config(
        *(material_config_t *)material_resc.data);

    resource_sys_unload(&material_resc);

    if (!m)
        ar_ERROR("Failed to load material.");

	return m;
}

material_t *material_sys_acquire_from_config(material_config_t config) {
    /* Return default material*/
    if (string_equali(config.name, DEFAULT_MATERIAL_NAME)) {
        return &p_state->default_material;
    }

    material_ref_t ref;
    if (p_state &&
        hashtable_get(&p_state->reg_material_table, config.name, &ref)) {
        if (ref.ref_count == 0) {
            ref.auto_release = config.auto_release;
        }
        ref.ref_count++;

        if (ref.handle == INVALID_ID) {
            uint32_t    count = p_state->config.max_material_count;
            material_t *mm    = 0;

            for (uint32_t i = 0; i < count; ++i) {
                if (p_state->reg_materials[i].id == INVALID_ID) {
                    ref.handle = i;
                    mm         = &p_state->reg_materials[i];
                    break;
                }
            }

            if (!mm || ref.handle == INVALID_ID) {
                ar_FATAL("material_sys_acquire_from_config - cannot hold "
                         "material anymore. Adjust configuration");
                return 0;
            }

            // Create new material
            if (!load_material(config, mm)) {
                ar_ERROR("Failed to load material '%s'", config.name);
                return 0;
            }

			shader_t *s = shader_sys_get_by_id(mm->shader_id);
            if (p_state->material_shader_id == INVALID_ID &&
                string_equal(config.shader_name,
                             BUILTIN_SHADER_NAME_MATERIAL)) {
                p_state->material_shader_id = s->id;
                p_state->material_loc.projection =
                    shader_sys_uniform_idx(s, "projection");
                p_state->material_loc.view = shader_sys_uniform_idx(s, "view");
                p_state->material_loc.diffuse_color =
                    shader_sys_uniform_idx(s, "diffuse_color");
                p_state->material_loc.diffuse_texture =
                    shader_sys_uniform_idx(s, "diffuse_texture");
                p_state->material_loc.model =
                    shader_sys_uniform_idx(s, "model");
            } else if (p_state->ui_shader_id == INVALID_ID &&
                       string_equal(config.shader_name,
                                    BUILTIN_SHADER_NAME_UI)) {
                p_state->ui_shader_id = s->id;
                p_state->ui_loc.projection =
                    shader_sys_uniform_idx(s, "projection");
                p_state->ui_loc.view = shader_sys_uniform_idx(s, "view");
                p_state->ui_loc.diffuse_color =
                    shader_sys_uniform_idx(s, "diffuse_color");
                p_state->ui_loc.diffuse_texture =
                    shader_sys_uniform_idx(s, "diffuse_texture");
                p_state->ui_loc.model = shader_sys_uniform_idx(s, "model");
            }

            if (mm->gen == INVALID_ID) {
                mm->gen = 0;
            } else {
                mm->gen++;
            }

            mm->id = ref.handle;
            ar_TRACE("Material '%s' does not exist yet. Create & ref_count is "
                     "%i",
                     config.diffuse_map_name, ref.ref_count);
        } else {
            ar_TRACE("Material '%s' already exists, ref_count increased to %i.",
                     config.name, ref.ref_count);
        }

        hashtable_set(&p_state->reg_material_table, config.name, &ref);
        return &p_state->reg_materials[ref.handle];
    }

    ar_ERROR("material_sys_acquire_from_config - Failed to aqcuire material "
             "'%s'. Null pointer return",
             config.name);
    return 0;
}

void material_sys_release(const char *name) {
    if (string_equali(name, DEFAULT_MATERIAL_NAME)) {
        return;
    }

    material_ref_t ref;
    if (p_state && hashtable_get(&p_state->reg_material_table, name, &ref)) {
        if (ref.ref_count == 0) {
            ar_WARNING("Tried to release non-existent material: '%s'", name);
            return;
        }
        ref.ref_count--;
        if (ref.ref_count == 0 && ref.auto_release) {
            material_t *m = &p_state->reg_materials[ref.handle];

            // Destroy/reset material.
            default_material_shut(m);

            // Reset the reference.
            ref.handle       = INVALID_ID;
            ref.auto_release = false;
            ar_TRACE("Released material '%s', Material unloaded because "
                     "reference count=0 and auto_release=true.",
                     name);
        } else {
            ar_TRACE("Released material '%s', now has a reference count of "
                     "'%i' (auto_release=%s).",
                     name, ref.ref_count, ref.auto_release ? "true" : "false");
        }

        // Update the entry.
        hashtable_set(&p_state->reg_material_table, name, &ref);
    } else {
        ar_ERROR("material_system_release failed to release material '%s'.",
                 name);
    }
}

#define MATERIAL_APPLY_OR_FAIL(expr)                                           \
    if (!expr) {                                                               \
        ar_ERROR("Failed to apply material: '%s'", expr);                      \
        return false;                                                          \
    }

b8 material_sys_apply_global(uint32_t shader_id, const mat4 *projection,
                             const mat4 *view) {
    if (shader_id == p_state->material_shader_id) {
        MATERIAL_APPLY_OR_FAIL(
            shader_sys_uniform_set_idx(p_state->material_loc.projection,
                                       projection));
        MATERIAL_APPLY_OR_FAIL(
            shader_sys_uniform_set_idx(p_state->material_loc.view, view));
    } else if (shader_id == p_state->ui_shader_id) {
        MATERIAL_APPLY_OR_FAIL(
            shader_sys_uniform_set_idx(p_state->ui_loc.projection, projection));
        MATERIAL_APPLY_OR_FAIL(
            shader_sys_uniform_set_idx(p_state->ui_loc.view, view));
    } else {
        ar_ERROR("material_sys_apply_global - Unrecognized shader id ''%d",
                 shader_id);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(shader_sys_apply_global());
    return true;
}

b8 material_sys_apply_instance(material_t *material) {
    MATERIAL_APPLY_OR_FAIL(shader_sys_bind_instance(material->internal_id));
    if (material->shader_id == p_state->material_shader_id) {
        MATERIAL_APPLY_OR_FAIL(
            shader_sys_uniform_set_idx(p_state->material_loc.diffuse_color,
                                       &material->diffuse_color));
        MATERIAL_APPLY_OR_FAIL(
            shader_sys_uniform_set_idx(p_state->material_loc.diffuse_texture,
                                       &material->diffuse_map.texture));
    } else if (material->shader_id == p_state->ui_shader_id) {
        MATERIAL_APPLY_OR_FAIL(
            shader_sys_uniform_set_idx(p_state->ui_loc.diffuse_color,
                                       &material->diffuse_color));
        MATERIAL_APPLY_OR_FAIL(
            shader_sys_uniform_set_idx(p_state->ui_loc.diffuse_texture,
                                       &material->diffuse_map.texture));
    } else {
        ar_ERROR("material_sys_apply_instance - Unrecognized shader id '%d' on "
                 "shader '%s'",
                 material->shader_id, material->name);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(shader_sys_apply_instance());
    return true;
}

b8 material_sys_apply_local(material_t *material, const mat4 *model) {
	if (material->shader_id == p_state->material_shader_id) {
		return shader_sys_uniform_set_idx(p_state->material_loc.model, model);
	} else if (material->shader_id == p_state->ui_shader_id) {
		return shader_sys_uniform_set_idx(p_state->ui_loc.model, model);
	}

	ar_ERROR("Unrecognized shader id '%d'", material->shader_id);
	return false;
}

