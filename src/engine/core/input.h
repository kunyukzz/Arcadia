#ifndef __INPUT_H__
#define __INPUT_H__

#include "engine/define.h"
#include "engine/core/keycode.h"

void input_init(uint64_t *memory_require, void *state);
void input_update(double delta_time);
void input_shut(void *state);

void input_process_key(keys key, b8 pressed);
void input_process_button(buttons button, b8 pressed);
void input_process_mouse_move(int16_t x, int16_t y);
void input_process_mouse_wheel(int8_t z_delta);

b8 input_keydown(keys key);
b8 input_keyup(keys key);
b8 input_was_keydown(keys key);
b8 input_was_keyup(keys key);

b8 input_mouse_button_down(buttons button);
b8 input_mouse_button_up(buttons button);
b8 input_mouse_was_button_down(buttons button);
b8 input_mouse_was_button_up(buttons button);
void input_get_mouse_pos(int32_t *x, int32_t *y);
void input_get_mouse_prev_pos(int32_t *x, int32_t *y);

#endif //__INPUT_H__
