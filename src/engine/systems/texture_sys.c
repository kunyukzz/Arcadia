#include "engine/systems/texture_sys.h"

#include "engine/container/hashtable.h"
#include "engine/core/logger.h"
#include "engine/core/ar_strings.h"
#include "engine/memory/memory.h"
#include "engine/renderer/renderer_fe.h"
#include "engine/systems/resource_sys.h"

typedef struct texture_sys_state_t {
	texture_sys_config_t config;
	texture_t default_texture;

	texture_t *reg_textures; // array of registered texture
	
	hashtable_t reg_texture_table; // hashtable for lookup
} texture_sys_state_t;

typedef struct texture_ref_t {
	uint64_t ref_count;
	uint32_t handle;
	b8 auto_release;
} texture_ref_t;

static texture_sys_state_t *p_state = 0;

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
b8 default_texture_init(texture_sys_state_t *state) {
    /* default texture. 256x256 */
    ar_TRACE("Create Default Texture...");
    const uint32_t tex_dimension = 256;
    const uint32_t channels      = 4;
    const uint32_t pixel_count   = tex_dimension * tex_dimension;
    uint8_t        pixels[pixel_count * channels];
    memory_set(pixels, 255, sizeof(uint8_t) * pixel_count * channels);

    // each pixel
    for (uint64_t row = 0; row < tex_dimension; ++row) {
        for (uint64_t col = 0; col < tex_dimension; ++col) {
            uint64_t idx         = (row * tex_dimension) + col;
            uint64_t channel_idx = idx * channels;

            uint8_t  checker =
                ((row / 32) % 2) ^ ((col / 32) % 2); // 32px squares
            if (checker) {
                pixels[channel_idx + 0] = 255; // R
                pixels[channel_idx + 1] = 0;   // G
                pixels[channel_idx + 2] = 255; // B
                pixels[channel_idx + 3] = 255; // A
            } else {
                pixels[channel_idx + 0] = 0;
                pixels[channel_idx + 1] = 0;
                pixels[channel_idx + 2] = 0;
                pixels[channel_idx + 3] = 255;
            }
        }
    }

    string_ncopy(state->default_texture.name, DEFAULT_TEXTURE_NAME,
                 TEXTURE_NAME_MAX_LENGTH);
    state->default_texture.gen             = INVALID_ID;
    state->default_texture.width           = tex_dimension;
    state->default_texture.height          = tex_dimension;
    state->default_texture.channel_count   = 4;
    state->default_texture.has_transparent = false;
    renderer_tex_init(pixels, &state->default_texture);

    state->default_texture.gen = INVALID_ID;
    return true;
}

void texture_shut(texture_t *t) {
	renderer_tex_shut(t);

	memory_zero(t->name, sizeof(char) * TEXTURE_NAME_MAX_LENGTH);
	memory_zero(t, sizeof(texture_t));

	t->id = INVALID_ID;
	t->gen = INVALID_ID;
}

void default_texture_shut(texture_sys_state_t *state) {
	if (state) {
		texture_shut(&state->default_texture);
	}
}

b8 load_texture(const char *texture_name, texture_t *tx) {
	resource_t image_resc;
	if (!resource_sys_load(texture_name, RESC_TYPE_IMAGE, &image_resc)) {
		ar_ERROR("Failed to load image resource for texture '%s'", texture_name);
		return false;
	}

	image_resc_data_t *resc_data = image_resc.data;
    texture_t temp;
	temp.width = resc_data->width;
	temp.height = resc_data->height;
	temp.channel_count = resc_data->channel_count;
	
	uint32_t curr_gen = tx->gen;
	tx->gen = INVALID_ID;

	uint64_t total_size = temp.width * temp.height * temp.channel_count;
	int has_transparent = false;
	for (uint64_t i = 0; i < total_size; ++i) {
		uint8_t a =resc_data->pixels[i + 3];

		if (a < 255) {
			has_transparent = true;
			break;
		}
	}

    // acquire internal texture resource & upload to gpu.
	string_ncopy(temp.name, texture_name, TEXTURE_NAME_MAX_LENGTH);
	temp.gen = INVALID_ID;
	temp.has_transparent = has_transparent;
	renderer_tex_init(resc_data->pixels, &temp);

	texture_t old = *tx;
	*tx           = temp;

	renderer_tex_shut(&old);
	if (curr_gen == INVALID_ID) {
		tx->gen = 0;
	} else {
		tx->gen = curr_gen + 1;
	}

	resource_sys_unload(&image_resc);
	return true;
}
/* ========================================================================== */
/* ========================================================================== */

b8 texture_sys_init(uint64_t *memory_require, void *state,
                    texture_sys_config_t config) {
    if (config.max_texture_count == 0) {
        ar_FATAL("Texture system failed. Count must be > 0.");
        return false;
    }

    uint64_t struct_req = sizeof(texture_sys_state_t);
    uint64_t array_req  = sizeof(texture_t) * config.max_texture_count;
    uint64_t table_req  = sizeof(texture_ref_t) * config.max_texture_count;
    *memory_require     = struct_req + array_req + table_req;

    if (!state) {
        return true;
    }

    p_state               = state;
    p_state->config       = config;

    // Array block is after state. Just set pointer
    void *array_block     = (char *)state + struct_req;
    p_state->reg_textures = array_block;

    // Hash block is after array
    void *hash_block      = (char *)array_block + array_req;

    hashtable_init(sizeof(texture_ref_t), config.max_texture_count, hash_block,
                   false, &p_state->reg_texture_table);

    // Set hashtable with invalid reference to use as default.
    texture_ref_t inv_ref;
    inv_ref.auto_release = false;
    inv_ref.handle       = INVALID_ID;
    inv_ref.ref_count    = 0;
    hashtable_fill(&p_state->reg_texture_table, &inv_ref);

    uint32_t count = p_state->config.max_texture_count;
    for (uint32_t i = 0; i < count; ++i) {
        p_state->reg_textures[i].id  = INVALID_ID;
        p_state->reg_textures[i].gen = INVALID_ID;
    }

    default_texture_init(p_state);
    return true;
}

void texture_sys_shut(void *state) {
	(void)state;
	if (p_state) {
		for (uint32_t i = 0; i < p_state->config.max_texture_count; ++i) {
			texture_t *t = &p_state->reg_textures[i];

			if (t->gen != INVALID_ID) {
				renderer_tex_shut(t);
			}
		}

		default_texture_shut(p_state);
		p_state = 0;
	}
}

texture_t *texture_sys_acquire(const char *name, b8 auto_release) {
    if (string_equali(name, DEFAULT_TEXTURE_NAME)) {
        ar_WARNING("Call for default texture. Use texture_sys_get_default_tex "
                   "for texture 'Default'.");
        return &p_state->default_texture;
    }

    texture_ref_t ref;
    if (p_state && hashtable_get(&p_state->reg_texture_table, name, &ref)) {
        if (ref.ref_count == 0) {
            ref.auto_release = auto_release;
        }
        ref.ref_count++;

        if (ref.handle == INVALID_ID) {
            uint32_t   c  = p_state->config.max_texture_count;
            texture_t *tt = 0;

            for (uint32_t i = 0; i < c; ++i) {
                if (p_state->reg_textures[i].id == INVALID_ID) {
                    ref.handle = i;
                    tt         = &p_state->reg_textures[i];
                    break;
                }
            }

            if (!tt || ref.handle == INVALID_ID) {
                ar_FATAL("Texture system cannot hold more texture. Adjust "
                         "configuration to allow more");
                return 0;
            }

            /* Cretate New Texture */
            if (!load_texture(name, tt)) {
                ar_ERROR("Failed to load texture '%s'", name);
                return 0;
            }

            /* Use handle as texture ID */
            tt->id = ref.handle;
            ar_TRACE("Texture '%s' not exist yet. Created, and ref_count is "
                     "now %i",
                     name, ref.ref_count);
        } else {
            ar_TRACE("Texture '%s' already exist. Ref_count increased to %i",
                     name, ref.ref_count);
        }

        hashtable_set(&p_state->reg_texture_table, name, &ref);
        return &p_state->reg_textures[ref.handle];
    }

    ar_ERROR("Failed to acquire texture '%s'. NULL pointer will returned",
             name);
    return 0;
}

void texture_sys_release(const char *name) {
    if (string_equali(name, DEFAULT_TEXTURE_NAME)) {
        return;
    }

    texture_ref_t ref;
    if (p_state && hashtable_get(&p_state->reg_texture_table, name, &ref)) {
        if (ref.ref_count == 0) {
            ar_WARNING("Tried to release Non-Exist texture: '%s'", name);
            return;
        }

		char name_copy[TEXTURE_NAME_MAX_LENGTH];
		string_ncopy(name_copy, name, TEXTURE_NAME_MAX_LENGTH);

        ref.ref_count--;
        if (ref.ref_count == 0 && ref.auto_release) {
            texture_t *tt = &p_state->reg_textures[ref.handle];

			texture_shut(tt);

            ref.handle       = INVALID_ID;
            ref.auto_release = false;
            ar_TRACE("Release texture '%s'. Ref_count=0 and auto_release=true",
                     name_copy);
        } else {
            ar_TRACE("Release texture '%s'. Ref_count=%i and auto_release=%s",
                     name_copy, ref.ref_count, ref.auto_release ? "true" : "false");
        }

        /* Update Entry */
        hashtable_set(&p_state->reg_texture_table, name_copy, &ref);
    } else {
        ar_ERROR("Texture system failed to release texture '%s'", name);
    }
}

texture_t *texture_sys_get_default_tex(void) {
    if (p_state) {
        return &p_state->default_texture;
        ar_INFO("Use default texture");
    }

    ar_ERROR("get_default_tex was called before texture system initialized. "
             "NULL pointer returned");
    return 0;
}
