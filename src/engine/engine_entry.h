#ifndef __ENGINE_ENTRY_H__
#define __ENGINE_ENTRY_H__

#include "engine/core/application.h"
#include "engine/core/logger.h"
#include "engine/engine.h"


/* this is the actual main for engine to be able to run. */

extern b8 game_entry_point(game_entry *game);

int main(void) {
	game_entry game_inst;

	if (!game_entry_point(&game_inst)) {
		ar_FATAL("Could not access game entry point");
		return -1;
	}

    if (!game_inst.init || !game_inst.run || !game_inst.render ||
        !game_inst.resize) {
        ar_FATAL("Game function pointer must be assign");
        return -2;
    }

    // Initialize Game
	if (!application_init(&game_inst)) {
		ar_FATAL("Application failed to initialize");
		return 1;
	}

	// Loop Game
	if (!application_run()) {
		ar_INFO("Application not properly shutdown");
		return -2;
	}

	return 0;
}

#endif //__ENGINE_ENTRY_H__
