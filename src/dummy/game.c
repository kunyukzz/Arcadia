#include "dummy/game.h"

#include "engine/core/logger.h"
#include "engine/memory/memory.h"

b8 game_init(game_entry *game_inst) {
	(void)game_inst;
	ar_DEBUG("game_init() called");

	return true;
}

b8 game_run(game_entry *game_inst, float delta_time) {
	(void)game_inst;
	(void)delta_time;

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
