#ifndef __GAME_H__
#define __GAME_H__

#include "engine/define.h"
#include "engine/engine.h"

#include "engine/math/math_type.h"

/* this is bridge inside dummy test for engine.
 * so, engine can running */

typedef struct game_state_t {
	float delta_time;
	mat4 view;
	vec3 cam_pos;
	vec3 cam_euler;
	b8 cam_view_dirty;
} game_state_t;

b8 game_init(game_entry *game_inst);
b8 game_run(game_entry *game_inst, float delta_time);
b8 game_render(game_entry *game_inst, float delta_time);
void game_resize(game_entry *game_inst, uint32_t width, uint32_t height);
void game_shut(game_entry *game_inst);

#endif //__GAME_H__
