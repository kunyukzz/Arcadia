#include "engine/renderer/renderer_fe.h"
#include "engine/renderer/renderer_be.h"

#include "engine/core/logger.h"
#include "engine/math/maths.h"

#include "engine/resources/resc_type.h"

typedef struct render_state_t {
	render_backend_t backend;
	mat4 projection;
	mat4 view;
	float near, far;

	texture_t default_texture;
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

    p_state->near       = 0.1f;
    p_state->far        = 1000.0f;
    p_state->projection = mat4_perspective(deg_to_rad(45.0f), 1280.0f / 720.0f,
                                           p_state->near, p_state->far);
    p_state->view       = mat4_translate((vec3){.x = 0.0f, 0.0f, -30.0f, 0.0f});
    p_state->view       = mat4_inverse(p_state->view);

	/* default texture. 256x256 */
	ar_TRACE("Create Default Texture...");
	const uint32_t tex_dimension = 256;
	const uint32_t channels = 4; 
	const uint32_t pixel_count = tex_dimension * tex_dimension;
	uint8_t pixels[pixel_count * channels];

	// each pixel
	for (uint64_t row = 0; row < tex_dimension; ++row) {
		for (uint64_t col = 0; col < tex_dimension; ++col) {
			uint64_t idx = (row * tex_dimension) + col;
			uint64_t channel_idx = idx * channels;

			uint8_t checker = ((row / 32) % 2) ^ ((col / 32) % 2); // 32px squares
			if (checker) {
				pixels[channel_idx + 0] = 255; 	// R
				pixels[channel_idx + 1] = 0; 	// G
				pixels[channel_idx + 2] = 255; 	// B
				pixels[channel_idx + 3] = 255; 	// A
			} else {
				pixels[channel_idx + 0] = 0;
				pixels[channel_idx + 1] = 0;
				pixels[channel_idx + 2] = 0;
				pixels[channel_idx + 3] = 255;
			}
		}
	}

    renderer_tex_init("Default", false, tex_dimension, tex_dimension, 4, pixels,
                      false, &p_state->default_texture);

    return true;
}

void renderer_shut(void *state) {
	(void)state;
	if (p_state) {
		renderer_tex_shut(&p_state->default_texture);
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
        p_state->projection =
            mat4_perspective(deg_to_rad(45.0f), width / (float)height,
                             p_state->near, p_state->far);
    } else {
        ar_WARNING("renderer backend does not exist to perform resize: %i %i",
                   width, height);
    }
}

b8 renderer_draw_frame(render_packet_t *packet) {
	if (renderer_begin_frame(packet->delta)) {
        p_state->backend.update_global(p_state->projection, p_state->view,
                                       vec3_zero(), vec4_one(), 0);
		
        static float angle = 0.01f;
		angle += 0.002f;
        quat rotation =
            quat_from_axis_angle((vec3){.x = 0.0f, 0.0f, 1.0f, 0.0f}, angle,
                                 false);
        mat4 model = quat_to_rotation_matrix(rotation, vec3_zero());

		//t4 model = mat4_translate(vec3_zero());
		geo_render_data_t data = {};
		data.obj_id = 0;
		data.model = model;
		data.textures[0] = &p_state->default_texture;
        p_state->backend.update_obj(data);

		b8 result = renderer_end_frame(packet->delta);
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

void renderer_tex_init(const char *name, b8 auto_release, int32_t width,
                       int32_t height, int32_t channel_count,
                       const uint8_t *pixel, b8 has_transparent,
                       texture_t *texture) {
    p_state->backend.init_tex(name, auto_release, width, height,
                                channel_count, pixel, has_transparent, texture);
}

void renderer_tex_shut(texture_t *texture) {
    p_state->backend.shut_tex(texture);
}
