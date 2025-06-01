#include "dummy/game.h"

#include "engine/engine_entry.h"

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

	game->state_memory_require = sizeof(game_state_t);
	game->state = 0;
	game->app_state = 0;

	return true;
}
