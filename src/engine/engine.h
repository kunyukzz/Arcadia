#ifndef __ENGINE_H__
#define __ENGINE_H__
#include <stdint.h>

#include "engine/core/application.h"

/* this is bridge inside engine. so game instance
 * using function pointer to be able to talk to engine. */

typedef struct game_entry {
	application_config_t app_config;

	b8 (*init)(struct game_entry *game_inst);
	b8 (*run)(struct game_entry *game_inst, float delta_time);
	b8 (*render)(struct game_entry *game_inst, float delta_time);
	void (*resize)(struct game_entry *game_inst, uint32_t width, uint32_t height);
	void (*shut)(struct game_entry *game_inst);

	void *state;
	void *app_state;
} game_entry;

#endif //__ENGINE_H__
