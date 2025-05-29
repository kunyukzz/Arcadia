#include "engine/systems/geometry_sys.h"

#include "engine/core/ar_strings.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"

#include "engine/renderer/renderer_fe.h"
#include "engine/systems/material_sys.h"
#include <stdint.h>

typedef struct geo_ref_t {
	uint64_t ref_count;
	geometry_t geometry;
	b8 auto_release;
} geo_ref_t;

typedef struct geometry_sys_state_t {
	geo_sys_cfg_t sys_cfg;
	geometry_t default_geometry;
	geometry_t default_2d_geometry;
	geo_ref_t *reg_geometry;
} geometry_sys_state_t;

static geometry_sys_state_t *p_state = 0;

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
b8 default_geo_init(geometry_sys_state_t *state) {
	vertex_3d verts[4];
	memory_zero(verts, sizeof(vertex_3d) * 4);

	/* Default Geometry */
	const float f = 10.0f;

	// Top Left
	verts[0].position.x = -0.5f * f;
	verts[0].position.y = -0.5f * f;
	verts[0].texcoord.x = 0.0f;
	verts[0].texcoord.y = 0.0f;

	// Bottom Right
	verts[1].position.x = 0.5f * f;
	verts[1].position.y = 0.5f * f;
	verts[1].texcoord.x = 1.0f;
	verts[1].texcoord.y = 1.0f;

	// Bottom Left
	verts[2].position.x = -0.5f * f;
	verts[2].position.y = 0.5f * f;
	verts[2].texcoord.x = 0.0f;
	verts[2].texcoord.y = 1.0f;

	// Top Right
	verts[3].position.x = 0.5f * f;
	verts[3].position.y = -0.5f * f;
	verts[3].texcoord.x = 1.0f;
	verts[3].texcoord.y = 0.0f;
	
	uint32_t indices[6] = {0, 1, 2, 0, 3, 1};

	/* Send geometry to renderer to be upload to GPU */
    if (!renderer_geometry_init(&state->default_geometry, sizeof(vertex_3d), 4,
                                verts, sizeof(uint32_t), 6, indices)) {
        ar_FATAL("Failed to create default geometry");
        return false;
    }

    state->default_geometry.material = material_sys_get_default();

	/* Default 2D Geometry */
	vertex_2d verts2d[4];
	memory_zero(verts2d, sizeof(vertex_2d) * 4);

	// Top Left
	verts2d[0].position.x = -0.5f * f;
	verts2d[0].position.y = -0.5f * f;
	verts2d[0].texcoord.x = 0.0f;
	verts2d[0].texcoord.y = 0.0f;

	// Bottom Right
	verts2d[1].position.x = 0.5f * f;
	verts2d[1].position.y = 0.5 * f;
	verts2d[1].texcoord.x = 1.0f;
	verts2d[1].texcoord.y = 1.0f;

	// Bottom Left
	verts2d[2].position.x = -0.5 * f;
	verts2d[2].position.y = 0.5 * f;
	verts2d[2].texcoord.x = 0.0f;
	verts2d[2].texcoord.y = 1.0f;

	// Top Right
	verts2d[3].position.x = 0.5f * f;
	verts2d[3].position.y = -0.5f * f;
	verts2d[3].texcoord.x = 1.0f;
	verts2d[3].texcoord.y = 0.0f;

	// NOTE: Counter-Clockwise
	//uint32_t indices2d[6] = {2, 1, 0, 3, 0, 1};
    uint32_t indices2d[6] = {0, 1, 2, 0, 3, 1};

	/* Send geometry to renderer to be upload to GPU */
    if (!renderer_geometry_init(&state->default_2d_geometry, sizeof(vertex_2d),
                                4, verts2d, sizeof(uint32_t), 6, indices2d)) {
        ar_FATAL("Failed to create default 2D geometry");
        return false;
    }

    state->default_2d_geometry.material = material_sys_get_default();

	return true;
}

b8 geo_init(geometry_sys_state_t *state, geo_config_t config, geometry_t *geo) {
    /* Send geometry to renderer to be upload to GPU */
    if (!renderer_geometry_init(geo, config.vertex_size, config.vertex_count,
                                config.vertices, config.idx_size,
                                config.idx_count, config.indices)) {
        state->reg_geometry[geo->id].ref_count    = 0;
        state->reg_geometry[geo->id].auto_release = false;
        geo->id                                   = INVALID_ID;
        geo->gen                                  = INVALID_ID;
        geo->internal_id                          = INVALID_ID;

        return false;
    }

    /* acquire material */
    if (string_length(config.material_name) > 0) {
        geo->material = material_sys_acquire(config.material_name);

        if (!geo->material) {
            geo->material = material_sys_get_default();
        }
    }

    return true;
}

void geo_shut(geometry_sys_state_t *state, geometry_t *geo) {
	(void)state;
	renderer_geometry_shut(geo);
	geo->internal_id = INVALID_ID;
	geo->gen = INVALID_ID;
	geo->id = INVALID_ID;

	string_empty(geo->name);

	/* release material */
	if (geo->material && string_length(geo->material->name) > 0) {
		material_sys_release(geo->material->name);
		geo->material = 0;
	}
}

/* ========================================================================== */
/* ========================================================================== */
b8 geometry_sys_init(uint64_t *memory_require, void *state,
                     geo_sys_cfg_t sys_cfg) {
    if (sys_cfg.max_geo_count == 0) {
        ar_FATAL("geometry_sys_init - max geometry must be > 0");
        return false;
    }

    /* Block of memory will contain state structure, then block for array, then
     * block for hashtable.*/
    uint64_t struct_req = sizeof(geometry_sys_state_t);
    uint64_t array_req  = sizeof(geo_ref_t) * sys_cfg.max_geo_count;
    *memory_require     = struct_req + array_req;

    if (!state) {
        return true;
    }

    p_state               = state;
    p_state->sys_cfg      = sys_cfg;

    /* The array block is after the state. Already allocated, so just set the
     * pointer. */
    void *array_block     = (char *)state + struct_req;
    p_state->reg_geometry = array_block;

    uint32_t count        = p_state->sys_cfg.max_geo_count;
    for (uint32_t i = 0; i < count; ++i) {
        p_state->reg_geometry[i].geometry.id          = INVALID_ID;
        p_state->reg_geometry[i].geometry.internal_id = INVALID_ID;
        p_state->reg_geometry[i].geometry.gen         = INVALID_ID;
    }

    if (!default_geo_init(p_state)) {
        ar_FATAL("Failed to create default geometry. Application stop");
        return false;
    }

    return true;
}

void geometry_sys_shut(void *state) {
	(void)state;
	// TODO: Nothing to do for geometry system shutdown.
}

geometry_t *geometry_sys_acquire_by_id(uint32_t id) {
    if (id != INVALID_ID &&
        p_state->reg_geometry[id].geometry.id != INVALID_ID) {
        p_state->reg_geometry[id].ref_count++;
        return &p_state->reg_geometry[id].geometry;
    }

    // NOTE: Should return default geometry instead?
    ar_ERROR("geometry_sys_acquire_by_id cannot load invalid geometry id. "
             "Returning nullptr.");
    return 0;
}

geometry_t *geometry_sys_acquire_by_config(geo_config_t config,
                                           b8           auto_release) {
    geometry_t *g = 0;
    for (uint32_t i = 0; i < p_state->sys_cfg.max_geo_count; ++i) {
        if (p_state->reg_geometry[i].geometry.id == INVALID_ID) {

            // found empty slot
            p_state->reg_geometry[i].auto_release = auto_release;
            p_state->reg_geometry[i].ref_count    = 1;
            g     = &p_state->reg_geometry[i].geometry;
            g->id = i;
            break;
        }
    }

    if (!g) {
        ar_ERROR("Unable to obtain free slot for geometry. Adjust "
                 "configuration to allow more space. Returning nullptr.");
        return 0;
    }

    if (!geo_init(p_state, config, g)) {
        ar_ERROR("Failed to create geometry. Returning nullptr.");
        return 0;
    }

    return g;
}

geometry_t *geometry_sys_get_default(void) {
    if (p_state) {
        return &p_state->default_geometry;
    }

    ar_FATAL("geometry_system_get_default called before system was "
             "initialized. Returning nullptr.");
    return 0;
}

geometry_t *geometry_sys_get_default_2d(void) {
    if (p_state)
        return &p_state->default_2d_geometry;

    ar_FATAL("geometry_system_get_default_2d called before system was "
             "initialized. Returning nullptr.");
    return 0;
}

void geometry_sys_release(geometry_t *geometry) {
    if (geometry && geometry->id != INVALID_ID) {
        geo_ref_t *ref = &p_state->reg_geometry[geometry->id];

        // take copy of ID
        uint32_t id    = geometry->id;
        if (ref->geometry.id == geometry->id) {
            if (ref->ref_count > 0) {
                ref->ref_count--;
            }

            // blank geometry id.
            if (ref->ref_count < 1 && ref->auto_release) {
                geo_shut(p_state, &ref->geometry);
                ref->ref_count    = 0;
                ref->auto_release = false;
            }
        } else {
            ar_FATAL("Geometry id mismatch. Check registration logic, as this "
                     "should never occur.");
        }

        return;
		(void)id;
    }

    ar_WARNING("geometry_sys_release cannot release invalid geometry "
               "id. Nothing was done.");
}

geo_config_t geometry_sys_gen_plane_config(float width, float height,
                                           uint32_t x_segcount,
                                           uint32_t y_segcount, float tile_x,
                                           float tile_y, const char *name,
                                           const char *material_name) {
    if (width == 0) {
        ar_WARNING("Width must be non zero. Set to 1");
        width = 1.0f;
    }

    if (height == 0) {
        ar_WARNING("Height must be non zero. Set to 1");
        height = 1.0f;
    }

    if (x_segcount < 1) {
        ar_WARNING("x_segcount must be positive number. Set to 1");
        x_segcount = 1;
    }

    if (y_segcount < 1) {
        ar_WARNING("y_segcount must be positive number. Set to 1");
        y_segcount = 1;
    }

    if (tile_x == 0) {
        ar_WARNING("tile_x must be non zero. Set to 1");
        tile_x = 1.0f;
    }

    if (tile_y == 0) {
        ar_WARNING("tile_y must be non zero. Set to 1");
        tile_y = 1.0f;
    }

    geo_config_t config;
	config.vertex_size = sizeof(vertex_3d);
    config.vertex_count = x_segcount * y_segcount * 4; // vertex per segment
	config.idx_size = sizeof(uint32_t);
    config.idx_count    = x_segcount * y_segcount * 6; // indices per segment
    config.vertices =
        memory_alloc(sizeof(vertex_3d) * config.vertex_count, MEMTAG_ARRAY);
    config.indices =
        memory_alloc(sizeof(uint32_t) * config.idx_count, MEMTAG_ARRAY);

    float seg_w  = width / x_segcount;
    float seg_h  = height / y_segcount;
    float half_w = width * 0.5f;
    float half_h = height * 0.5f;
    for (uint32_t y = 0; y < y_segcount; ++y) {
        for (uint32_t x = 0; x < x_segcount; ++x) {

            // generate vertices
            float      min_x    = (x * seg_w) - half_w;
            float      min_y    = (y * seg_h) - half_h;
            float      max_x    = min_x + seg_w;
            float      max_y    = min_y + seg_h;
            float      min_uvx  = (x / (float)x_segcount) * tile_x;
            float      min_uvy  = (y / (float)y_segcount) * tile_y;
            float      max_uvx  = ((x + 1) / (float)x_segcount) * tile_x;
            float      max_uvy  = ((y + 1) / (float)y_segcount) * tile_y;

            uint32_t   v_offset = ((y * x_segcount) + x) * 4;
            vertex_3d *v0       = &((vertex_3d *)config.vertices)[v_offset + 0];
            vertex_3d *v1       = &((vertex_3d *)config.vertices)[v_offset + 1];
            vertex_3d *v2       = &((vertex_3d *)config.vertices)[v_offset + 2];
            vertex_3d *v3       = &((vertex_3d *)config.vertices)[v_offset + 3];

            v0->position.x      = min_x;
            v0->position.y      = min_y;
            v0->texcoord.x      = min_uvx;
            v0->texcoord.y      = min_uvy;

            v1->position.x      = max_x;
            v1->position.y      = max_y;
            v1->texcoord.x      = max_uvx;
            v1->texcoord.y      = max_uvy;

            v2->position.x      = min_x;
            v2->position.y      = max_y;
            v2->texcoord.x      = min_uvx;
            v2->texcoord.y      = max_uvy;

            v3->position.x      = max_x;
            v3->position.y      = min_y;
            v3->texcoord.x      = max_uvx;
            v3->texcoord.y      = min_uvy;

            // generate indices
            uint32_t i_offset   = ((y * x_segcount) + x) * 6;
			((uint32_t *)config.indices)[i_offset + 0] = v_offset + 0;
			((uint32_t *)config.indices)[i_offset + 1] = v_offset + 1;
			((uint32_t *)config.indices)[i_offset + 2] = v_offset + 2;
			((uint32_t *)config.indices)[i_offset + 3] = v_offset + 0;
			((uint32_t *)config.indices)[i_offset + 4] = v_offset + 3;
			((uint32_t *)config.indices)[i_offset + 5] = v_offset + 1;
        }
    }

    if (name && string_length(name) > 0) {
        string_ncopy(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    } else {
        string_ncopy(config.name, DEFAULT_GEOMETRY_NAME,
                     GEOMETRY_NAME_MAX_LENGTH);
    }

    if (material_name && string_length(material_name) > 0) {
        string_ncopy(config.material_name, material_name,
                     MATERIAL_NAME_MAX_LENGTH);
    } else {
        string_ncopy(config.material_name, DEFAULT_MATERIAL_NAME,
                     MATERIAL_NAME_MAX_LENGTH);
    }

    return config;
}
