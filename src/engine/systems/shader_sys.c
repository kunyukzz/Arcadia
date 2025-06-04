#include "engine/systems/shader_sys.h"

#include "engine/container/dyn_array.h"
#include "engine/core/ar_strings.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"
#include "engine/renderer/renderer_fe.h"
#include "engine/systems/texture_sys.h"

typedef struct shader_sys_state_t {
	shader_sys_config_t config;
	shader_t *shader;
	hashtable_t lookup;
	uint32_t curr_shader_id;
	void *lookup_memory;
} shader_sys_state_t;

static shader_sys_state_t *p_state = 0;

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
b8 uniform_name_valid(shader_t *shader, const char *uniform_name) {
    if (!uniform_name || !string_length(uniform_name)) {
        ar_ERROR("Uniform name must exist");
        return false;
    }

    uint16_t location;
    if (hashtable_get(&shader->uniform_lookup, uniform_name, &location) &&
        location != INVALID_ID_U16) {
        ar_ERROR("A uniform by the name '%s' already exist on '%s'",
                 uniform_name, shader->name);
        return false;
    }
    return true;
}

b8 shader_uniform_add_state_valid(shader_t *shader) {
	if (shader->state != SHADER_UNINITIALIZED) {
		ar_ERROR("Uniforms may only be added so shader before initialization");
		return false;
	}
	return true;
}

b8 uniform_add(shader_t *shader, const char *uniform_name, uint32_t size,
               shader_uniform_type_t type, shader_scope_t scope,
               uint32_t set_loc, b8 is_sampler) {
    uint32_t uniform_count = dyn_array_length(shader->uniforms);
    if (uniform_count + 1 > p_state->config.max_uniform_count) {
        ar_ERROR("A shader only accept a combined max of %d uniform & sampler "
                 "at global, instance and local scope",
                 p_state->config.max_uniform_count);
        return false;
    }

    shader_uniform_t entry;
	entry.index = uniform_count;
	entry.scope = scope;
	entry.type = type;
	b8 is_global = (scope == SHADER_SCOPE_GLOBAL);
	if (is_sampler) {
		entry.location = set_loc;
	} else {
		entry.location = entry.index;
	}

	if (scope != SHADER_SCOPE_LOCAL) {
		entry.set_index = (uint32_t)scope;
        entry.offset    = is_sampler  ? 0
                      : is_global ? shader->global_ubo_size
                                      : shader->ubo_size;
        entry.size      = is_sampler ? 0 : size;
    } else {
		if (entry.scope == SHADER_SCOPE_LOCAL && !shader->use_locals) {
            ar_ERROR("Cannot add local-scoped uniform for a shader that "
                     "doesn't support local");
            return false;
        }

		/* Push new aligned range (align to 4, as required Vulkan Spec */
		entry.set_index = INVALID_ID_U8;
		range r = get_aligned_range(shader->push_const_size, size, 4);
		entry.offset = r.offset;
		entry.size = r.size;

		shader->push_const_ranges[shader->push_const_range_count] = r;
		shader->push_const_range_count++;
		shader->push_const_size += r.size;
	}

	if (!hashtable_set(&shader->uniform_lookup, uniform_name, &entry.index)) {
		ar_ERROR("Failed to add uniform");
		return false;
	}
	dyn_array_push(shader->uniforms, entry);

	if (!is_sampler) {
		if (entry.scope == SHADER_SCOPE_GLOBAL) {
			shader->global_ubo_size += entry.size;
		} else if (entry.scope == SHADER_SCOPE_INSTANCE) {
			shader->ubo_size += entry.size;
		}
	}

	return true;
}

b8 add_attr(shader_t *shader, const shader_attr_config_t *config) {
	uint32_t size = 0;
	switch (config->type) {
		case SHADER_ATTR_INT8:
		case SHADER_ATTR_UINT8:
			size = 1;
			break;
		case SHADER_ATTR_INT16:
		case SHADER_ATTR_UINT16:
			size = 2;
			break;
		case SHADER_ATTR_FLOAT32:
		case SHADER_ATTR_INT32:
		case SHADER_ATTR_UINT32:
			size = 4;
			break;
		case SHADER_ATTR_FLOAT32_2:
			size = 8;
			break;
		case SHADER_ATTR_FLOAT32_3:
			size = 12;
			break;
		case SHADER_ATTR_FLOAT32_4:
			size = 16;
			break;
		default:
			ar_ERROR("Unrecognized type %d, default size to 4. Not desired");
			size = 4;
			break;
	}
	shader->attr_stride += size;

	/* Push the attributes */
	shader_attribute_t attrib = {};
	attrib.name = string_duplicate(config->name);
	attrib.size = size;
	attrib.type = config->type;
	dyn_array_push(shader->attributes, attrib);

	return true;
}

b8 add_sampler(shader_t *shader, shader_uniform_config_t *config) {
    if (config->scope == SHADER_SCOPE_INSTANCE && !shader->use_instances) {
        ar_ERROR("add_sampler - cannot add instance sampler for shader isn't "
                 "use instance");
        return false;
    }

    if (config->scope == SHADER_SCOPE_LOCAL) {
        ar_ERROR("add_sampler - cannot add sampler at local scope");
        return false;
    }

    if (!uniform_name_valid(shader, config->name) ||
        !shader_uniform_add_state_valid(shader)) {
        return false;
    }

    uint32_t location = 0;
    if (config->scope == SHADER_SCOPE_GLOBAL) {
        uint32_t global_tex_count = dyn_array_length(shader->global_textures);

        if (global_tex_count + 1 > p_state->config.max_global_texture) {
            ar_ERROR("Shader global texture count %i exceed max of %i",
                     shader->inst_texture_count,
                     p_state->config.max_global_texture);
            return false;
        }

        location = global_tex_count;
        dyn_array_push(shader->global_textures, texture_sys_get_default_tex());
    } else {

        if (shader->inst_texture_count + 1 >
            p_state->config.max_instance_texture) {
            ar_ERROR("Shader instance texture count %i exceed max of %i",
                     shader->inst_texture_count,
                     p_state->config.max_instance_texture);
            return false;
        }
        location = shader->inst_texture_count;
        shader->inst_texture_count++;
    }

    if (!uniform_add(shader, config->name, 0, config->type, config->scope,
                     location, true)) {
        ar_ERROR("Unable to add sampler uniform");
        return false;
    }

    return true;
}

b8 add_uniform(shader_t *shader, shader_uniform_config_t *config) {
    if (!shader_uniform_add_state_valid(shader) ||
        !uniform_name_valid(shader, config->name)) {
        return false;
    }
    return uniform_add(shader, config->name, config->size, config->type,
                       config->scope, 0, false);
}

uint32_t get_shader_id(const char *shader_name) {
	uint32_t shader_id = INVALID_ID;
	if (!hashtable_get(&p_state->lookup, shader_name, &shader_id)) {
		ar_ERROR("No shader registered name '%s'", shader_name);
		return INVALID_ID;
	}
	return shader_id;
}

uint32_t new_shader_id(void) {
	for (uint32_t i = 0; i < p_state->config.max_shader_count; ++i) {
		if (p_state->shader[i].id == INVALID_ID) {
			return i;
		}
	}
	return INVALID_ID;
}

void shader_shut(shader_t *shader) {
	renderer_shader_shut(shader);

	shader->state = SHADER_NOT_CREATED;

	if (shader->name) {
		uint32_t length = string_length(shader->name);
		memory_free(shader->name, length + 1, MEMTAG_STRING);
	}
	shader->name = 0;
}

void shader_system_shut_internal(const char *shader_name) {
	uint32_t shader_id = get_shader_id(shader_name);
	if (shader_id == INVALID_ID) {
		return;
	}
	shader_t *s = &p_state->shader[shader_id];
	shader_shut(s);
}

/* ========================================================================== */
/* ========================================================================== */
b8 shader_sys_init(uint64_t *memory_require, void *memory,
                   shader_sys_config_t config) {
    if (config.max_shader_count < 512) {
        if (config.max_shader_count == 0) {
            ar_ERROR("shader_sys_init - config.max_shader_count must greater "
                     "than 0");
            return false;
        } else {
            ar_WARNING("shader_sys_init - config.max_shader_count is recommend "
                       "at least 512");
        }
    }

    /* Figure out how large of a hashtable is needed.
	 * Block of memory will contain state structure then the block 
	 * for hashtable*/
	uint64_t struct_req = sizeof(shader_sys_state_t);
	uint64_t hash_req = sizeof(uint32_t) * config.max_shader_count;
	uint64_t shader_array_req = sizeof(shader_t) * config.max_shader_count;
	*memory_require = struct_req + hash_req * shader_array_req;

	if (!memory) {
		return true;
	}

	/* Setup p_state pointer, memory block, shader array, then hashtable */
	p_state = memory;
	uint64_t addr = (uint64_t)memory;
	p_state->lookup_memory = (void *)(addr + struct_req);
	p_state->shader = (void *)((uint64_t)p_state->lookup_memory + hash_req);
	p_state->config = config;
	p_state->curr_shader_id = INVALID_ID;
    hashtable_init(sizeof(uint32_t), config.max_shader_count,
                   p_state->lookup_memory, false, &p_state->lookup);

    for (uint32_t i = 0; i < config.max_shader_count; ++i) {
		p_state->shader[i].id = INVALID_ID;
	}

	uint32_t invalid_fill_id = INVALID_ID;
	if (!hashtable_fill(&p_state->lookup, &invalid_fill_id)) {
		ar_ERROR("hashtable_fill failed");
		return false;
	}

	for (uint32_t i = 0; i < p_state->config.max_shader_count; ++i) {
		p_state->shader[i].id = INVALID_ID;
	}

	return true;
}

void shader_sys_shut(void *state) {
	if (state) {
		shader_sys_state_t *st = (shader_sys_state_t *)state;
		for (uint32_t i = 0; i < st->config.max_shader_count; ++i) {
			shader_t *s = &st->shader[i];
			if (s->id != INVALID_ID)
				shader_shut(s);
		}
		hashtable_shut(&st->lookup);
		memory_zero(st, sizeof(shader_sys_state_t));
	}

	p_state = 0;
}

b8 shader_sys_create(const shader_config_t *config) {
	uint32_t id = new_shader_id();
	shader_t *out_shader = &p_state->shader[id];
	memory_zero(out_shader, sizeof(shader_t));
	out_shader->id = id;
	if (out_shader->id == INVALID_ID) {
		ar_ERROR("Unable to find free slot to create new shader. Abort..");
		return false;
	}

	out_shader->state = SHADER_NOT_CREATED;
	out_shader->name = string_duplicate(config->name);
	out_shader->use_instances = config->use_instances;
	out_shader->use_locals = config->use_locals;
	out_shader->push_const_range_count = 0;
	memory_zero(out_shader->push_const_ranges, sizeof(range) * 32);
	out_shader->bound_inst_id = INVALID_ID;
	out_shader->attr_stride = 0;

	/* Setup Arrays */
	out_shader->global_textures = dyn_array_create(texture_t *);
	out_shader->uniforms = dyn_array_create(shader_uniform_t);
	out_shader->attributes = dyn_array_create(shader_attribute_t);

	/* Create hashtable to store uniform array indexes. This provide a direct
	 * index into the 'uniforms' array stored in the shader for quick lookups by
	 * name. */
	uint64_t el_size = sizeof(uint16_t);
	uint64_t el_count = 1024;
	out_shader->hashtable_block = memory_alloc(el_size * el_count, MEMTAG_UNKNOWN);
    hashtable_init(el_size, el_count, out_shader->hashtable_block, false,
                   &out_shader->uniform_lookup);

    uint32_t invalid = INVALID_ID;
	hashtable_fill(&out_shader->uniform_lookup, &invalid);

	out_shader->global_ubo_size = 0;
	out_shader->ubo_size = 0;
	// NOTE: UBO alignment requirement set in renderer backend

	/* This is hard-coded because Vulkan spec only guarantee that minimum 128
	 * bytes of space are available, and it's up to the driver to determine how
	 * much is available. Therefore, to avoid complexity, only the lowest common
	 * denominator of 128B will be used */
	out_shader->push_const_stride = 128;
	out_shader->push_const_size = 0;

	uint8_t renderlayer_id = INVALID_ID_U8;
	if (!renderer_renderpass_id(config->renderpass_name, &renderlayer_id)) {
		ar_ERROR("Unable to find renderpass '%s'", config->renderpass_name);
		return false;
	}

    if (!renderer_shader_create(out_shader, renderlayer_id, config->stage_count,
                                (const char **)config->stage_filenames,
                                config->stages)) {
        ar_ERROR("Error creating shader...");
        return false;
    }

	// Ready to be Initialize
	out_shader->state = SHADER_UNINITIALIZED;

	// Attribute 
	for (uint32_t i = 0; i < config->attr_count; ++i) {
		add_attr(out_shader, &config->attributes[i]);
	}

	// Uniforms
	for (uint32_t i = 0; i < config->uniform_count; ++i) {
		if (config->uniforms[i].type == SHADER_UNIFORM_SAMPLER) {
            add_sampler(out_shader, &config->uniforms[i]);
        } else {
			add_uniform(out_shader, &config->uniforms[i]);
		}
    }

	// Initialize Shader
	if (!renderer_shader_init(out_shader)) {
        ar_ERROR("shader_sys_create - Initialization failed for shader '%s'",
                 config->name);
        return false;
    }

	if (!hashtable_set(&p_state->lookup, config->name, &out_shader->id)) {
		renderer_shader_shut(out_shader);
		return false;
	}

    return true;
}

uint32_t shader_sys_get_id(const char *shader_name) {
	return get_shader_id(shader_name);
}

shader_t *shader_sys_get_by_id(uint32_t shader_id) {
    if (shader_id >= p_state->config.max_shader_count ||
        p_state->shader[shader_id].id == INVALID_ID) {
        return 0;
    }
    return &p_state->shader[shader_id];
}

shader_t *shader_sys_get(const char *shader_name) {
	uint32_t shader_id = get_shader_id(shader_name);
	if (shader_id != INVALID_ID) {
		return shader_sys_get_by_id(shader_id);
	}
	return 0;
}

b8 shader_sys_use(const char *shader_name) {
	uint32_t next = get_shader_id(shader_name);
	if (next == INVALID_ID)
		return false;

	return shader_sys_use_id(next);
}

b8 shader_sys_use_id(uint32_t shader_id) {
	if (p_state->curr_shader_id != shader_id) {
		shader_t *next = shader_sys_get_by_id(shader_id);
		p_state->curr_shader_id = shader_id;

		if (!renderer_shader_use(next)) {
			ar_ERROR("Failed to use shader '%s'", next->name);
			return false;
		}

		if (!renderer_shader_bind_global(next)) {
			ar_ERROR("Failed to bind global for shader '%s'", next->name);
			return false;
		}
	}
	return true;
}

uint16_t shader_sys_uniform_idx(shader_t *shader, const char *uniform_name) {
    if (!shader || shader->id == INVALID_ID) {
        ar_ERROR("shader_sys_uniform_idx - called with invalid shader.");
		return INVALID_ID_U16;
    }

    uint16_t idx = INVALID_ID_U16;
    if (!hashtable_get(&shader->uniform_lookup, uniform_name, &idx) ||
        idx == INVALID_ID_U16) {
        ar_ERROR("Shader '%s' doesn't have registered uniform name '%s'",
                 shader->name, uniform_name);
        return INVALID_ID_U16;
    }
    return shader->uniforms[idx].index;
}

b8 shader_sys_uniform_set(const char *uniform_name, const void *value) {
	if (p_state->curr_shader_id == INVALID_ID) {
		ar_ERROR("shader_sys_uniform_set - called without shader in use");
		return false;
	}

	shader_t *s = &p_state->shader[p_state->curr_shader_id];
	uint16_t idx = shader_sys_uniform_idx(s, uniform_name);
	return shader_sys_uniform_set_idx(idx, value);
}

b8 shader_sys_uniform_set_idx(uint16_t index, const void *value) {
	shader_t *shader = &p_state->shader[p_state->curr_shader_id];
	shader_uniform_t *uni = &shader->uniforms[index];

	if (shader->bound_scope != uni->scope) {
		if (uni->scope == SHADER_SCOPE_GLOBAL) {
			renderer_shader_bind_global(shader);
		} else if (uni->scope == SHADER_SCOPE_INSTANCE) {
			renderer_shader_bind_instance(shader, shader->bound_inst_id);
		} else {
			// NOTE: Nothing todo for local.
		}
		shader->bound_scope = uni->scope;
	}
	return renderer_set_uniform(shader, uni, value);
}

b8 shader_sys_sampler_set(const char *sampler_name, const texture_t *tex) {
    return shader_sys_uniform_set(sampler_name, tex);
}

b8 shader_sys_sampler_set_idx(uint16_t index, const struct texture_t *tex) {
    return shader_sys_uniform_set_idx(index, tex);
}

b8 shader_sys_apply_global() {
    return renderer_shader_apply_global(
        &p_state->shader[p_state->curr_shader_id]);
}

b8 shader_sys_apply_instance() {
    return renderer_shader_apply_instance(
        &p_state->shader[p_state->curr_shader_id]);
}

b8 shader_sys_bind_instance(uint32_t instance_id) {
	shader_t *s = &p_state->shader[p_state->curr_shader_id];
	s->bound_inst_id = instance_id;
	return renderer_shader_bind_instance(s, instance_id);
}

