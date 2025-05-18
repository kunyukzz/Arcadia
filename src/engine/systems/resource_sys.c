#include "engine/systems/resource_sys.h"
#include "engine/core/logger.h"
#include "engine/core/ar_strings.h"

#include "engine/resources/text_loader.h"
#include "engine/resources/binary_loader.h"
#include "engine/resources/image_loader.h"
#include "engine/resources/material_loader.h"

typedef struct resource_sys_state_t {
	resource_sys_config_t config;
	resource_loader_t *reg_loaders;
} resource_sys_state_t;

static resource_sys_state_t *p_state = 0;

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
b8 load(const char *name, resource_loader_t *loader, resource_t *resc) {
	if (!name || !loader || !loader->load || !resc) {
		resc->id_loader = INVALID_ID;
		return false;
	}

	resc->id_loader = loader->id;
	return loader->load(loader, name, resc);
}
/* ========================================================================== */
/* ========================================================================== */

b8 resource_sys_init(uint64_t *memory_require, void *state,
                     resource_sys_config_t config) {
    if (config.max_loader_count == 0) {
        ar_FATAL("resource_sys_init - failed because max count was 0");
        return false;
    }

    *memory_require = sizeof(resource_sys_state_t) +
                      (sizeof(resource_loader_t) * config.max_loader_count);

    if (!state) return true;
    p_state              = state;
    p_state->config      = config;

    void *array_block    = (char *)state + sizeof(resource_sys_state_t);
    p_state->reg_loaders = array_block;

    uint32_t count       = config.max_loader_count;
    for (uint32_t i = 0; i < count; ++i) {
        p_state->reg_loaders[i].id = INVALID_ID;
    }

    // TODO: auto register known loader
	resource_sys_reg_loader(loader_text_rsc_init());
	resource_sys_reg_loader(loader_binary_rsc_init());
	resource_sys_reg_loader(loader_image_rsc_init());
	resource_sys_reg_loader(loader_material_rsc_init());

    ar_INFO("Resource System Initialize. Base path: '%s'", config.base_path);
    return true;
}

void resource_sys_shut(void *state) {
	(void)state;
	if (p_state) p_state = 0;
}

b8 resource_sys_reg_loader(resource_loader_t loader) {
    if (!p_state) return false;

    uint32_t count = p_state->config.max_loader_count;

    for (uint32_t i = 0; i < count; ++i) {
        resource_loader_t *l = &p_state->reg_loaders[i];
        if (l->id != INVALID_ID) {
            if (l->type == loader.type) {
                ar_ERROR("resource_sys_reg_loader - loader of type "
                         "%d already exists and will not be registered.",
                         loader.type);
                return false;
            } else if (loader.custom_type &&
                       string_length(loader.custom_type) > 0 &&
                       string_equali(l->custom_type, loader.custom_type)) {
                ar_ERROR("resource_sys_reg_loader - loader of custom type %s "
                         "already exists & will not be registered",
                         loader.custom_type);
                return false;
            }
        }
    }

    for (uint32_t j = 0; j < count; ++j) {
        if (p_state->reg_loaders[j].id == INVALID_ID) {
            p_state->reg_loaders[j]    = loader;
            p_state->reg_loaders[j].id = j;
            ar_TRACE("Loader Registered");
            return true;
        }
    }

    return false;
}

b8 resource_sys_load(const char *name, resource_type_t type, resource_t *resc) {
	if (p_state && type != RESC_TYPE_CUSTOM) {
		
		// Select loader
		uint32_t count = p_state->config.max_loader_count;
		for (uint32_t i = 0; i < count; ++i) {
			resource_loader_t *l = &p_state->reg_loaders[i];
			if (l->id != INVALID_ID && l->type == type) {
				return load(name, l, resc);
			}
		}
	}

	resc->id_loader = INVALID_ID;
    ar_ERROR("resource_system_load - No loader for type %d was found.", type);
	return false;
}

b8 resource_sys_load_custom(const char *name, const char *custom_type,
                            resource_t *resc) {
    if (p_state && custom_type && string_length(custom_type) > 0) {

        // Select loader.
        uint32_t count = p_state->config.max_loader_count;
        for (uint32_t i = 0; i < count; ++i) {
            resource_loader_t *l = &p_state->reg_loaders[i];
            if (l->id != INVALID_ID && l->type == RESC_TYPE_CUSTOM &&
                string_equali(l->custom_type, custom_type)) {
                return load(name, l, resc);
            }
        }
    }

    resc->id_loader = INVALID_ID;
    return false;
}

void resource_sys_unload(resource_t *resc) {
    if (p_state && resc) {
        if (resc->id_loader != INVALID_ID) {
            resource_loader_t *l = &p_state->reg_loaders[resc->id_loader];

            if (l->id != INVALID_ID && l->unload) {
                l->unload(l, resc);
            }
        }
    }
}

const char *resource_sys_base_path(void) {
    if (p_state)
        return p_state->config.base_path;

    return "";
}
