#include "engine/core/input.h"
#include "engine/core/event.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"

#define KEYS_TOTAL 256

typedef struct keyboard_state_t {
	b8 keys[KEYS_TOTAL];
} keyboard_state_t;

typedef struct mouse_state_t {
	int16_t mouse_x, mouse_y;
	int8_t mouse_wheel_delta;
	uint8_t buttons[BUTTON_MAX];
} mouse_state_t;

typedef struct input_state_t {
	keyboard_state_t kbd_current;
	keyboard_state_t kbd_prev;
	mouse_state_t mouse_current;
	mouse_state_t mouse_prev;
} input_state_t;

static input_state_t *p_state;

void input_init(uint64_t *memory_require, void *state) {
	*memory_require = sizeof(input_state_t);
	if (state == 0)
		return;

	memory_zero(state, sizeof(input_state_t));
	p_state = state;
	ar_INFO("Input System Initialized");
}

void input_update(double delta_time) {
	(void)delta_time;

	if (!p_state)
		return;

	memory_copy(&p_state->kbd_prev, &p_state->kbd_current,
				sizeof(keyboard_state_t));
	memory_copy(&p_state->mouse_prev, &p_state->mouse_current,
				sizeof(mouse_state_t));
	p_state->mouse_current.mouse_wheel_delta = 0;
}

void input_shut(void *state) {
	(void)state;
	p_state = 0;
}

void input_process_key(keys key, b8 pressed) {
	if (p_state && p_state->kbd_current.keys[key] != pressed) {
		p_state->kbd_current.keys[key] = pressed;

		event_context_t ec;
		ec.data.u16[0] = key;
		event_push(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASE, 0, ec);
	}
}

void input_process_button(buttons button, b8 pressed) {
	if (p_state->mouse_current.buttons[button] != pressed) {
		p_state->mouse_current.buttons[button] = pressed;

		event_context_t ec;
		ec.data.u16[0] = button;
		event_push(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASE, 0, ec);
		
	}
}

void input_process_mouse_move(int16_t x, int16_t y) {
	if (p_state->mouse_current.mouse_x != x || p_state->mouse_current.mouse_y != y) {
		p_state->mouse_current.mouse_x = x;
		p_state->mouse_current.mouse_y = y;

		event_context_t ec;
		ec.data.u16[0] = (uint16_t)x;
		ec.data.u16[1] = (uint16_t)y;
		event_push(EVENT_CODE_MOUSE_MOVE, 0, ec);
	}
}

void input_process_mouse_wheel(int8_t z_delta) {
	p_state->mouse_current.mouse_wheel_delta = z_delta;

	if (z_delta) {
		event_context_t ec;
		ec.data.u8[0] = (uint8_t)z_delta;
		event_push(EVENT_CODE_MOUSE_WHEEL, 0, ec);
	}
}

// ----------------------------- KEYBOARD INPUT -------------------------------
b8 input_keydown(keys key) {
	if (!p_state)
		return false;

	return p_state->kbd_current.keys[key] == true;
}

b8 input_keyup(keys key) {
	if (!p_state)
		return true;

	return p_state->kbd_current.keys[key] == false;
}

b8 input_was_keydown(keys key) {
	if (!p_state)
		return false;

	return p_state->kbd_prev.keys[key] == true;
}

b8 input_was_keyup(keys key) {
	if (!p_state)
		return true;

	return p_state->kbd_prev.keys[key] == false;
}

// ------------------------------- MOUSE INPUT -------------------------------
b8 input_mouse_button_down(buttons button) {
	if (!p_state)
		return false;

	return p_state->mouse_current.buttons[button] == true;
}

b8 input_mouse_button_up(buttons button) {
	if (!p_state)
		return true;

	return p_state->mouse_current.buttons[button] == false;
}

b8 input_mouse_was_button_down(buttons button) {
	if (!p_state)
		return false;

	return p_state->mouse_prev.buttons[button] == true;
}

b8 input_mouse_was_button_up(buttons button) {
	if (!p_state)
		return true;

	return p_state->mouse_prev.buttons[button] == false;
}

void input_get_mouse_pos(int32_t *x, int32_t *y) {
	if (!p_state) {
		*x = 0;
		*y = 0;
		return;
	}

	*x = p_state->mouse_current.mouse_x;
	*y = p_state->mouse_current.mouse_y;
}

void input_get_mouse_prev_pos(int32_t *x, int32_t *y) {
	if (!p_state) {
		*x = 0;
		*y = 0;
		return;
	}

	*x = p_state->mouse_prev.mouse_x;
	*y = p_state->mouse_prev.mouse_y;
}

