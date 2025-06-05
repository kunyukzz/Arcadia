#include "engine/renderer/renderer_fe.h"
#include "engine/renderer/renderer_be.h"

#include "engine/core/ar_strings.h"
#include "engine/core/logger.h"
#include "engine/math/maths.h"
#include "engine/resources/resc_type.h"
#include "engine/systems/resource_sys.h"
#include "engine/systems/shader_sys.h"
#include "engine/systems/material_sys.h"

typedef struct render_state_t {
	render_backend_t backend;
	mat4 projection;
	mat4 view;
	mat4 ui_projection;
	mat4 ui_view;
	float near, far;
	uint32_t material_shader_id;
	uint32_t ui_shader_id;
} render_state_t;

#define CRITICAL_INIT(op, msg)                                                 \
    if (!op) {                                                                 \
        ar_ERROR(msg);                                                           \
        return false;                                                          \
    }

static render_state_t *p_state;

void mat4_print(mat4 m) {
    for (int i = 0; i < 4; ++i) {
        ar_TRACE("[%.2f %.2f %.2f %.2f]",
               m.data[i * 4 + 0],
               m.data[i * 4 + 1],
               m.data[i * 4 + 2],
               m.data[i * 4 + 3]);
    }
}

b8 renderer_init(uint64_t *memory_require, void *state, const char *name) {
	*memory_require = sizeof(render_state_t);
	if (state == 0)
		return true;
	p_state = state;

	renderer_be_init(BACKEND_VULKAN, &p_state->backend);
	p_state->backend.frame_number = 0;

    CRITICAL_INIT(p_state->backend.init(&p_state->backend, name),
                  "Renderer backend failed to initialize. Shut.");
	/* Shader */
	resource_t config_resc;
	shader_config_t *config = 0;

    CRITICAL_INIT(resource_sys_load(BUILTIN_SHADER_NAME_MATERIAL,
                                    RESC_TYPE_SHADER, &config_resc),
                  "Failed to load builtin material shader");
	config = (shader_config_t *)config_resc.data;
	CRITICAL_INIT(shader_sys_create(config), "Failed to load builtin material shader");
	resource_sys_unload(&config_resc);
	p_state->material_shader_id = shader_sys_get_id(BUILTIN_SHADER_NAME_MATERIAL);

	CRITICAL_INIT(
        resource_sys_load(BUILTIN_SHADER_NAME_UI, RESC_TYPE_SHADER, &config_resc),
        "Failed to load builtin UI shader.");
    config = (shader_config_t *)config_resc.data;
    CRITICAL_INIT(shader_sys_create(config), "Failed to load builtin UI shader.");
    resource_sys_unload(&config_resc);
    p_state->ui_shader_id = shader_sys_get_id(BUILTIN_SHADER_NAME_UI);

    /* World Projection */
    p_state->near       = 0.1f;
    p_state->far        = 1000.0f;
    p_state->projection = mat4_perspective(deg_to_rad(45.0f), 1280.0f / 720.0f,
                                           p_state->near, p_state->far);
    p_state->view       = mat4_translate((vec3){.x = 0.0f, 0.0f, -30.0f, 0.0f});
    p_state->view       = mat4_inverse(p_state->view);
 
	/* UI Projection */
	p_state->ui_projection = mat4_ortho(0.0f, 1280.0f, 0.0f, 720.0f, -100.0f, 100.0f);
	p_state->ui_view = mat4_identity();
	/*
	mat4 m = mat4_ortho(0, 1280, 0, 720, -100, 100);
    for (int row = 0; row < 4; ++row) {
        ar_DEBUG("[ ");
        for (int col = 0; col < 4; ++col) {
            ar_DEBUG("%.2f ", m.data[col * 4 + row]);
        }
        ar_DEBUG("]\n");
    }
	*/

	/*
    ar_TRACE("UI Projection: ");
    mat4_print(p_state->ui_projection);

    ar_TRACE("UI View: ");
    mat4_print(p_state->ui_view);
	*/

    return true;
}

void renderer_shut(void *state) {
	(void)state;
	if (p_state) {
		p_state->backend.shut(&p_state->backend);
	}

	renderer_be_shut(&p_state->backend);
	p_state = 0;
}

void renderer_resize(uint32_t width, uint32_t height) {
	if (p_state) {
        p_state->projection =
            mat4_perspective(deg_to_rad(45.0f), width / (float)height,
                             p_state->near, p_state->far);
        p_state->ui_projection =
            mat4_ortho(0, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
		p_state->backend.resize(&p_state->backend, width, height);
    } else {
        ar_WARNING("renderer backend does not exist to perform resize: %i %i",
                   width, height);
    }
}

b8 renderer_draw_frame(render_packet_t *packet) {
    if (p_state->backend.begin_frame(&p_state->backend, packet->delta)) {

        /* World Render Layer */
        if (!p_state->backend.begin_renderpass(&p_state->backend,
                                               RENDER_LAYER_WORLD)) {
            ar_ERROR("backend.begin_renderpass -> RENDER_LAYER_WORLD failed. "
                     "Application shutting down...");
            return false;
        }
		if (!shader_sys_use_id(p_state->material_shader_id)) {
			ar_ERROR("Failed to use material shader. Render frame failed");
			return false;
		}

        if (!shader_sys_apply_global(p_state->material_shader_id,
                                     &p_state->projection, &p_state->view)) {
            ar_ERROR("Failed to use apply globals for material shader. Render "
                     "frame failed.");
            return false;
        }

        uint32_t count = packet->geo_count;
        for (uint32_t i = 0; i < count; ++i) {
			material_t* m = 0;
            if (packet->geometries[i].geometry->material) {
                m = packet->geometries[i].geometry->material;
            } else {
                m = material_sys_get_default();
            }

            // Apply the material
            if (!material_sys_apply_instance(m)) {
                ar_WARNING("Failed to apply material '%s'. Skipping draw.", m->name);
                continue;
            }

            // Apply the locals
            material_sys_apply_local(m, &packet->geometries[i].model);
            p_state->backend.draw_geometry(packet->geometries[i]);
        }

        if (!p_state->backend.end_renderpass(&p_state->backend,
                                            RENDER_LAYER_WORLD)) {
            ar_ERROR("backend.end_renderpass -> RENDER_LAYER_WORLD failed. "
                     "Application shutting down...");
            return false;
        }

        /* UI Render Layer */
        if (!p_state->backend.begin_renderpass(&p_state->backend,
                                               RENDER_LAYER_UI)) {
            ar_ERROR("backend.begin_renderpass -> RENDER_LAYER_UI failed. "
                     "Application shutting down...");
            return false;
        }
		if (!shader_sys_use_id(p_state->ui_shader_id)) {
			ar_ERROR("Failed to use UI Shader. Render frame failed");
			return false;
		}

		if (!material_sys_apply_global(p_state->ui_shader_id, &p_state->ui_projection, &p_state->ui_view)) {
			ar_ERROR("Failed to use apply global for UI Shader. Render frame failed.");
			return false;
		}
		
        count = packet->ui_geo_count;
        for (uint32_t i = 0; i < count; ++i) {
			material_t* m = 0;
            if (packet->ui_geometries[i].geometry->material) {
                m = packet->ui_geometries[i].geometry->material;
            } else {
                m = material_sys_get_default();
            }
            // Apply the material
            if (!material_sys_apply_instance(m)) {
                ar_WARNING("Failed to apply UI material '%s'. Skipping draw.", m->name);
                continue;
            }

            // Apply the locals
            material_sys_apply_local(m, &packet->geometries[i].model);
            p_state->backend.draw_geometry(packet->ui_geometries[i]);
        }

        if (!p_state->backend.end_renderpass(&p_state->backend,
                                            RENDER_LAYER_UI)) {
            ar_ERROR("backend.end_renderpass -> RENDER_LAYER_UI failed. "
                     "Application shutting down...");
            return false;
        }

        b8 result =
            p_state->backend.end_frame(&p_state->backend, packet->delta);
        p_state->backend.frame_number++;

        if (!result) {
            ar_ERROR("renderer_end_frame failed. Shut");
            return false;
        }
    }
    return true;
}

void renderer_set_view(mat4 view) {
	p_state->view = view;
}

void renderer_tex_init(const uint8_t *pixel, texture_t *texture) {
    p_state->backend.init_tex(pixel, texture);
}

void renderer_tex_shut(texture_t *texture) {
    p_state->backend.shut_tex(texture);
}

b8 renderer_geometry_init(geometry_t *geometry, uint32_t vertex_size,
                          uint32_t vertex_count, const void *vertices,
                          uint32_t idx_size, uint32_t idx_count,
                          const void *indices) {
    return p_state->backend.init_geo(geometry, vertex_size, vertex_count,
                                     vertices, idx_size, idx_count, indices);
}

void renderer_geometry_shut(geometry_t *geometry) {
	p_state->backend.shut_geo(geometry);
}

b8 renderer_renderpass_id(const char *name, uint8_t *renderpass_id) {
	// TODO: HACK: Need dynamic renderpasses instead of hardcoding them.
    if (string_equali("Renderpass.Builtin.World", name)) {
        *renderpass_id = RENDER_LAYER_WORLD;
        return true;
    } else if (string_equali("Renderpass.Builtin.UI", name)) {
        *renderpass_id = RENDER_LAYER_UI;
        return true;
    }

    ar_ERROR("renderer_renderpass_id: No renderpass named '%s'.", name);
    *renderpass_id = INVALID_ID_U8;
    return false;
}

b8 renderer_shader_create(struct shader_t *shader, uint8_t renderpass_id,
                          uint8_t stage_count, const char **stage_filenames,
                          shader_stage_t *stage) {
    return p_state->backend.create_shader(shader, renderpass_id, stage_count,
                                          stage_filenames, stage);
}

void renderer_shader_shut(struct shader_t *shader) {
	p_state->backend.shut_shader(shader);
}

b8 renderer_shader_init(struct shader_t *shader) {
	return p_state->backend.init_shader(shader);
}

b8 renderer_shader_use(struct shader_t *shader) {
	return p_state->backend.use_shader(shader);
}

b8 renderer_shader_bind_global(struct shader_t *shader) {
	return p_state->backend.bind_shader_global(shader);
}

b8 renderer_shader_bind_instance(struct shader_t *shader, uint32_t instance_id) {
	return p_state->backend.bind_shader_instance(shader, instance_id);
}

b8 renderer_shader_apply_global(struct shader_t *shader) {
	return p_state->backend.apply_shader_global(shader);
}

b8 renderer_shader_apply_instance(struct shader_t *shader) {
	return p_state->backend.apply_shader_instance(shader);
}

b8 renderer_shader_acquire_inst_resc(struct shader_t *shader, uint32_t *instance_id) {
	return p_state->backend.acquire_shader_inst_resc(shader, instance_id);
}

b8 renderer_shader_release_inst_resc(struct shader_t *shader, uint32_t instance_id) {
	return p_state->backend.release_shader_inst_resc(shader, &instance_id);
}

b8 renderer_set_uniform(struct shader_t *shader, struct shader_uniform_t *uniform, const void *value) {
	return p_state->backend.set_uniform_shader(shader, uniform, value);
}

