#include "engine/engine.h"

#include "engine/core/application.h"
#include "engine/core/logger.h"
#include "engine/core/event.h"
#include "engine/core/input.h"
#include "engine/core/strings.h"

#include "engine/memory/memory.h"
#include "engine/memory/arena.h"

#include "engine/platform/platform_time.h"
#include "engine/platform/platform.h"

#include "engine/renderer/renderer_fe.h"

typedef struct subsys_state_t {
	uint64_t size;
	void *state;
} subsys_state_t;

typedef struct application_state_t {
	game_entry *game_inst;
	b8 is_running;
	b8 is_suspend;
	uint32_t width;
	uint32_t height;

	platform_time_t time;
	int16_t last_time;

	arena_allocator_t arena;

	subsys_state_t event;
	subsys_state_t memory;
	subsys_state_t log;
	subsys_state_t input;
	subsys_state_t platform;
	subsys_state_t renderer;

} application_state_t;

static application_state_t *p_state;


/* 				 this is part of internal event handling for engine 		  */
/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */

b8 app_on_event(uint16_t code, void *sender, void *listener, event_context_t ec) {
	(void)sender;
	(void)listener;
	(void)ec;

	switch (code) {
		case EVENT_CODE_APPLICATION_QUIT: {
			ar_INFO("EVENT_CODE_APPLICATION_QUIT received. Shutdown.");
			p_state->is_running = false;
			return true;
		}
		case EVENT_CODE_APP_SUSPEND: {
			ar_INFO("EVENT_CODE_APP_SUSPEND received. Suspend.");
			p_state->is_suspend = true;
			return true;
		}
		case EVENT_CODE_APP_RESUME: {
			ar_INFO("EVENT_CODE_APP_RESUME received. Resume.");
			p_state->is_suspend = false;
			p_state->is_running = true;
			return true;
		}
	}
	
	return false;
}

b8 app_on_key(uint16_t code, void *sender, void *listener, event_context_t ec) {
	(void)sender;
	(void)listener;

	if (code == EVENT_CODE_KEY_PRESSED) {
		uint16_t kc = ec.data.u16[0];
		if (kc == KEY_ESCAPE) {
			event_context_t data = {};
			event_push(EVENT_CODE_APPLICATION_QUIT, 0, data);
			return true;
		} else {
			ar_DEBUG("'%c' key pressed in window", kc);
		}
	} else if (code == EVENT_CODE_KEY_RELEASE) {
		uint16_t kc = ec.data.u16[0];
		if (kc == KEY_SPACE) {
			ar_DEBUG("Explicit! 'Space' key is released");
		} else {
			ar_DEBUG("'%c' key released in window", kc);
		}
	}

	return false;
}

b8 app_on_resized(uint16_t code, void *sender, void *listener, event_context_t ec) {
	(void)sender;
	(void)listener;

	if (code == EVENT_CODE_RESIZED) {
		uint32_t w = ec.data.u16[0];
		uint32_t h = ec.data.u16[1];

		if (w != p_state->width || h != p_state->height) {
			p_state->width = w;
			p_state->height = h;

			if (w == 0 || h == 0) {
				ar_INFO("Window minimized. Suspend application");
				p_state->is_suspend = true;
				return true;
			} else {
				if (p_state->is_suspend) {
					ar_INFO("Window restored. Resume application");
					p_state->is_suspend = false;
				}
				p_state->game_inst->resize(p_state->game_inst, w, h);
				renderer_resize(w, h);
			}
		}
	}

	return false;
}

/* ========================================================================== */
/* ========================================================================== */

b8 application_init(struct game_entry *game_inst) {
	if (game_inst->app_state) {
		ar_ERROR("Engine already running");
		return false;
	}

	game_inst->app_state = memory_alloc(sizeof(*p_state), MEMTAG_APPLICATION);
	p_state = game_inst->app_state;
	p_state->game_inst = game_inst;
	p_state->is_running = false;
	p_state->is_suspend = false;

	/* set chunk of memory allocation */
	uint64_t total_size_alloc = 64 * 1024 * 1024; // 64 Mb
	arena_init(total_size_alloc, 0, &p_state->arena);

	/* Set event memory allocation */
	event_init(&p_state->event.size, 0);
	p_state->event.state = arena_allocate(&p_state->arena, p_state->event.size);
	event_init(&p_state->event.size, p_state->event.state);

	/* set memory allocation */
	memory_init(&p_state->memory.size, 0);
	p_state->memory.state = arena_allocate(&p_state->arena, p_state->memory.size);
	memory_init(&p_state->memory.size, p_state->memory.state);

	/* set log memory allocation */
	log_init(&p_state->log.size, 0);
	p_state->log.state = arena_allocate(&p_state->arena, p_state->log.size);
	log_init(&p_state->log.size, p_state->log.state);

	/* set input memory allocation */
	input_init(&p_state->input.size, 0);
	p_state->input.state = arena_allocate(&p_state->arena, p_state->input.size);
	input_init(&p_state->input.size, p_state->input.state);

	event_reg(EVENT_CODE_APPLICATION_QUIT, 0, app_on_event);
	event_reg(EVENT_CODE_KEY_PRESSED, 0, app_on_key);
	event_reg(EVENT_CODE_KEY_RELEASE, 0, app_on_key);
	event_reg(EVENT_CODE_RESIZED, 0, app_on_resized);
	event_reg(EVENT_CODE_APP_SUSPEND, 0, app_on_event);
	event_reg(EVENT_CODE_APP_RESUME, 0, app_on_event);

	/* set platform memory allocation */
	platform_init(&p_state->platform.size, 0, 0, 0, 0, 0, 0);
	p_state->platform.state =
		arena_allocate(&p_state->arena, p_state->platform.size);
	if (!platform_init(&p_state->platform.size, p_state->platform.state,
					   game_inst->app_config.name,
					   game_inst->app_config.pos_x,
					   game_inst->app_config.pos_y,
					   game_inst->app_config.width,
					   game_inst->app_config.height)) return false;

	/* set renderer memory allocation */
	renderer_init(&p_state->renderer.size, 0, 0);
	p_state->renderer.state =
		arena_allocate(&p_state->arena, p_state->renderer.size);
	if (!renderer_init(&p_state->renderer.size, p_state->renderer.state,
						  game_inst->app_config.name)) {
	  ar_FATAL("Renderer failed to initialize");
	  return false;
	}

	/* game start */
	if (!p_state->game_inst->init(p_state->game_inst)) {
		ar_FATAL("Game failed to initialize");
		return false;
	}

	p_state->game_inst->resize(p_state->game_inst, p_state->width,
							   p_state->height);

	return true;
}

b8 application_run(void) {
	/* engine start loop */
	p_state->is_running = true;
	time_start(&p_state->time);
	time_update(&p_state->time);
	p_state->last_time = p_state->time.elapsed;

	double runtime = 0;
	uint8_t frame_count = 0;
	float target_frame_seconds = 1.0f / 60;

	char *stats = memory_debug_stats();
	ar_INFO(stats);
	memory_free(stats, string_length(stats) + 1, MEMTAG_STRING);

	while (p_state->is_running) {
		if (!platform_push())
			p_state->is_running = false;

		if (!p_state->is_suspend) {
			time_update(&p_state->time);
			double current_time = p_state->time.elapsed;

			//float delta = (float)(current_time - p_state->last_time);
			double delta = (current_time - p_state->last_time);
			p_state->last_time = current_time;

			double frame_start_time = get_absolute_time();

			if (!p_state->game_inst->run(p_state->game_inst, (float)delta)) {
				ar_FATAL("Game run failed");
				p_state->is_running = false;
				break;
			}

			if (!p_state->game_inst->render(p_state->game_inst, (float)delta)) {
				ar_FATAL("Game render failed");
				p_state->is_running = false;
				break;
			}

			render_packet_t packet;
			packet.delta = delta;
			renderer_draw_frame(&packet);

			/* calculate how long frame took */
			double frame_end_time = get_absolute_time();
			double frame_elapsed_time = frame_end_time - frame_start_time;
			runtime += frame_elapsed_time;
			double remain_seconds = target_frame_seconds - frame_elapsed_time;
			if (remain_seconds > 0) {
				uint64_t remain_ms = (remain_seconds * 1000);
				b8 limit = false;
				if (remain_ms > 0 && limit) {
					//os_sleep(remain_ms);
					os_sleep(remain_ms - 1);
					ar_DEBUG("platform sleep");
				}
				frame_count++;
			}

			input_update(delta);
			p_state->last_time = current_time;
			

			/* hahahahaa */
			//ar_TRACE("runtime: %f, frame_count: %u", runtime, frame_count);
			(void)runtime;
			(void)frame_count;
		}
	}

	/* engine end loop */
	p_state->is_running = false;
	
	event_unreg(EVENT_CODE_APPLICATION_QUIT, 0, app_on_event);
	event_unreg(EVENT_CODE_KEY_PRESSED, 0, app_on_key);
	event_unreg(EVENT_CODE_KEY_RELEASE, 0, app_on_key);
	event_unreg(EVENT_CODE_RESIZED, 0, app_on_resized);
	event_unreg(EVENT_CODE_APP_SUSPEND, 0, app_on_event);
	event_unreg(EVENT_CODE_APP_RESUME, 0, app_on_event);
	

	input_shut(p_state->input.state);
	renderer_shut(p_state->renderer.state);
	platform_shut(p_state->platform.state);
	log_shut(p_state->log.state);
	memory_shut(p_state->memory.state);
	event_shut(p_state->event.state);

	if (p_state->game_inst->shut)
		p_state->game_inst->shut(p_state->game_inst);

	return true;
}

void application_get_framebuffer_size(uint32_t *width, uint32_t *height) {
	*width = p_state->width;
	*height = p_state->height;
}

