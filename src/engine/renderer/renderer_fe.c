#include "engine/renderer/renderer_fe.h"
#include "engine/renderer/renderer_be.h"

#include "engine/core/logger.h"
#include "engine/math/maths.h"
#include "engine/resources/resc_type.h"

typedef struct render_state_t {
	render_backend_t backend;
	mat4 projection;
	mat4 view;
	mat4 ui_projection;
	mat4 ui_view;
	float near, far;
} render_state_t;

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
	if (!p_state->backend.init(&p_state->backend, name)) {
		ar_FATAL("Render Backend cannot initialized");
		return false;
	}

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
        p_state->backend.update_world(p_state->projection, p_state->view,
                                      vec3_zero(), vec4_one(), 0);

        uint32_t count = packet->geo_count;
        for (uint32_t i = 0; i < count; ++i) {
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
        p_state->backend.update_ui(p_state->ui_projection, p_state->ui_view, 0);

        count = packet->ui_geo_count;
        for (uint32_t i = 0; i < count; ++i) {
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

b8 renderer_material_init(material_t *material) {
	return p_state->backend.init_material(material);
}

void renderer_material_shut(material_t *material) {
	p_state->backend.shut_material(material);
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

