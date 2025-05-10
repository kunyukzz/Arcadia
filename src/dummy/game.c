#include "dummy/game.h"

#include "engine/core/logger.h"
#include "engine/core/input.h"
#include "engine/core/event.h"
#include "engine/memory/memory.h"

// NOTE: should not available outside engine.
#include "engine/renderer/renderer_fe.h"
#include "engine/math/maths.h"

void recalc_view_matrix(game_state_t *p_state) {
    if (p_state->cam_view_dirty) {
        mat4 rotation =
            mat4_euler_xyz(p_state->cam_euler.x, p_state->cam_euler.y,
                           p_state->cam_euler.z);
        mat4 translate          = mat4_translate(p_state->cam_pos);

        // p_state->view = mat4_multi(translate, rotation);
        p_state->view           = mat4_multi(rotation, translate);
        p_state->view           = mat4_inverse(p_state->view);

        p_state->cam_view_dirty = false;
    }
}

void cam_yaw(game_state_t *p_state, float amount) {
	p_state->cam_euler.y += amount;
	p_state->cam_view_dirty = true;
}

void cam_pitch(game_state_t *p_state, float amount) {
	p_state->cam_euler.x += amount;

	float limit = deg_to_rad(89.0f);
	p_state->cam_euler.x = ar_CLAMP(p_state->cam_euler.x, -limit, limit);

	p_state->cam_view_dirty = true;
}

b8 game_init(game_entry *game_inst) {
	game_state_t *p_state = (game_state_t *)game_inst->state;

	p_state->cam_pos = (vec3){.x = 0.0f, 0.0f, 60.0f, 0.0f};
	p_state->cam_euler = vec3_zero();

	p_state->view = mat4_translate(p_state->cam_pos);
	p_state->view = mat4_inverse(p_state->view);

	p_state->cam_view_dirty = true;
	return true;
}

b8 game_run(game_entry *game_inst, float delta_time) {

	static uint64_t alloc_count = 0;
	uint64_t prev_alloc_count = alloc_count;

    if (input_keyup('M') && input_was_keydown('M')) {
        ar_DEBUG("Allocs: %llu (%llu this frame)", alloc_count,
                 alloc_count - prev_alloc_count);
    }

    // TODO: Temporary
	if (input_keyup('T') && input_was_keydown('T')) {
		ar_DEBUG("Switch Texture");
		event_context_t ev = {0};
		event_push(EVENT_CODE_DEBUG0, 0, ev);
	}

	game_state_t *p_state = (game_state_t *)game_inst->state;

	// movement camera
	float gg = 0.01f;
    if (input_keydown('Q') || input_keydown(KEY_LEFT)) {
        cam_yaw(p_state, gg * delta_time);
	}
    if (input_keydown('E') || input_keydown(KEY_RIGHT)) {
        cam_yaw(p_state, -gg * delta_time);
	}
    if (input_keydown(KEY_UP)) {
        cam_pitch(p_state, gg * delta_time);
	}
    if (input_keydown(KEY_DOWN)) {
        cam_pitch(p_state, -gg * delta_time);
	}

    float temp = 1.0f;
    vec3 velo = vec3_zero();
	if (input_keydown('F')) {
		p_state->cam_pos = (vec3){.x = 0.0f, 0.0f, 60.0f, 0.0f};
		p_state->cam_euler = vec3_zero();
		p_state->view = mat4_translate(p_state->cam_pos);
		p_state->view = mat4_inverse(p_state->view);
		p_state->cam_view_dirty = true;
	}

    if (input_keydown('W')) {
        vec3 forward = mat4_forward(p_state->view);
        velo         = vec3_add(velo, forward);
    }
    if (input_keydown('S')) {
        vec3 backward = mat4_backward(p_state->view);
        velo          = vec3_add(velo, backward);
    }
    if (input_keydown('A')) {
        vec3 left = mat4_left(p_state->view);
        velo      = vec3_add(velo, left);
    }
    if (input_keydown('D')) {
        vec3 right = mat4_right(p_state->view);
        velo       = vec3_add(velo, right);
    }

    if (input_keydown('Z')) {
        velo.y += 0.01f;
	}
    if (input_keydown('X')) {
        velo.y -= 0.01f;
	}

    vec3 z = vec3_zero();
    if (!vec3_compared(z, velo, 0.0002f)) {
        vec3_normalized(&velo);
        p_state->cam_pos.x += velo.x * temp * delta_time;
        p_state->cam_pos.y += velo.y * temp * delta_time;
        p_state->cam_pos.z += velo.z * temp * delta_time;

        p_state->cam_view_dirty = true;
    }

	recalc_view_matrix(p_state);

	// NOTE: should not available outside engine.
	renderer_set_view(p_state->view);

	return true;
}

b8 game_render(game_entry *game_inst, float delta_time) {
	(void)game_inst;
	(void)delta_time;
	return true;
}

void game_resize(game_entry *game_inst, uint32_t width, uint32_t height) {
	(void)game_inst;
	(void)width;
	(void)height;
}

void game_shut(game_entry *game_inst) {
	if (game_inst->state) {
		memory_free(game_inst->state, sizeof(game_state_t), MEMTAG_GAME);
		game_inst->state = 0;
	}
}
