#include "engine/renderer/renderer_fe.h"
#include "engine/renderer/renderer_be.h"

#include "engine/core/logger.h"
#include "engine/math/maths.h"

typedef struct render_state_t {
	render_backend_t backend;
	float near, far;
} render_state_t;

static render_state_t *p_state;

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

	p_state->near = 0.1f;
	p_state->far = 1000.0f;

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

b8 renderer_begin_frame(float delta_time) {
	if (!p_state)
		return false;

	return p_state->backend.begin_frame(&p_state->backend, delta_time);
}

b8 renderer_end_frame(float delta_time) {
	if (!p_state)
		return false;

	b8 result = p_state->backend.end_frame(&p_state->backend, delta_time);
	p_state->backend.frame_number++;
	return result;
}

void renderer_resize(uint32_t width, uint32_t height) {
	if (p_state) {
		p_state->backend.resize(&p_state->backend, width, height);	
	} else {
        ar_WARNING("renderer backend does not exist to perform resize: %i %i",
                   width, height);
    }
}

b8 renderer_draw_frame(render_packet_t *packet) {
	if (renderer_begin_frame(packet->delta)) {
        mat4 projection = mat4_perspective(deg_to_rad(45.0f), 1280.0f / 720.0f,
                                          0.1f, 1000.0f);
		static float z = 0.0f;
		z -= 0.01f;
		mat4 view = mat4_translate((vec3){{0.0f, 0.0f, z}});

        p_state->backend.update_global(projection, view,
                                       vec3_zero(), vec4_one(), 0);

		static float angle = 0.01f;
		angle += 0.01f;
		quat rotation = quat_from_axis_angle((vec3){{0.0f, 0.0f, 1.0f, 0.0f}}, angle, false);
		mat4 model = quat_to_rotation_matrix(rotation, vec3_zero());
		//mat4 model = mat4_translation((vec3){{0.0f, 0.0f, 0.0f}});
		p_state->backend.update_obj(model);

		b8 result = renderer_end_frame(packet->delta);
        if (!result) {
			ar_ERROR("renderer_end_frame failed. Shut");
			return false;
		}
	}
	return true;
}

