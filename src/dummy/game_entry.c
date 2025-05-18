#include "dummy/game.h"

#include "engine/engine_entry.h"
#include "engine/memory/memory.h"

/* this function was expected by the engine.
 * so the engine knows what game want to do*/

b8 game_entry_point(game_entry *game) {
	game->app_config.width = 1280;
	game->app_config.height = 720;
	game->app_config.name = "Arcadia Engine";

	game->init = game_init;
	game->run = game_run;
	game->render = game_render;
	game->resize = game_resize;
	game->shut = game_shut;

	game->state = memory_alloc(sizeof(game_state_t), MEMTAG_GAME);
	game->app_state = 0;

	return true;
}
