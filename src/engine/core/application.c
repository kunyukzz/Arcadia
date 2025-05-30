/* This should be include first before anything
else since platform_time using _POSIX_C_SOURCE. */
#include "engine/platform/platform_time.h"

#include "engine/engine.h"

#include "engine/core/application.h"
#include "engine/core/logger.h"
#include "engine/core/event.h"
#include "engine/core/input.h"
#include "engine/core/ar_strings.h"
#include "engine/memory/memory.h"
#include "engine/memory/arena.h"
#include "engine/platform/platform.h"
#include "engine/renderer/renderer_fe.h"

#include "engine/systems/texture_sys.h"
#include "engine/systems/material_sys.h"
#include "engine/systems/geometry_sys.h"
#include "engine/systems/resource_sys.h"

// TODO: Temporary
#include "engine/math/maths.h"

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
	subsys_state_t resources;
	subsys_state_t renderer;
	subsys_state_t textures;
	subsys_state_t material;
	subsys_state_t geometry;

	// TODO: Temporary
	geometry_t *test_geo;
	geometry_t *test_ui_geo;

} application_state_t;

static application_state_t *p_state;


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

b8 app_on_debug(uint16_t code, void *sender, void *listener,
                event_context_t ec) {
	(void)code;
	(void)sender;
	(void)listener;
	(void)ec;

    const char   *names[3] = {"cobblestone", "paving", "paving2"};
    static int8_t choice   = 2;
    const char   *old      = names[choice];
    choice++;
    choice %= 3;

    if (p_state->test_geo) {
        p_state->test_geo->material->diffuse_map.texture =
            texture_sys_acquire(names[choice], true);

        if (!p_state->test_geo->material->diffuse_map.texture) {
            ar_WARNING("app_on_debug - no texture! use default one");
            p_state->test_geo->material->diffuse_map.texture =
                texture_sys_get_default_tex();
        }

        texture_sys_release(old);
    }

    return true;
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
	event_reg(EVENT_CODE_DEBUG0, 0, app_on_debug);

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

	/* Resources System */
	resource_sys_config_t resc_sys_config;
	resc_sys_config.base_path = "../assets";
	resc_sys_config.max_loader_count = 32;
	resource_sys_init(&p_state->resources.size, 0, resc_sys_config);
    p_state->resources.state =
        arena_allocate(&p_state->arena, p_state->resources.size);
    if (!resource_sys_init(&p_state->resources.size, p_state->resources.state,
                           resc_sys_config)) {
        ar_FATAL("Failed to initialize resource.");
        return false;
    }

    /* set renderer memory allocation */
	renderer_init(&p_state->renderer.size, 0, 0);
	p_state->renderer.state =
		arena_allocate(&p_state->arena, p_state->renderer.size);
	if (!renderer_init(&p_state->renderer.size, p_state->renderer.state,
						  game_inst->app_config.name)) {
	  ar_FATAL("Renderer failed to initialize");
	  return false;
	}

    /* set texture memory allocation */
    texture_sys_config_t tex_sys_config;
    tex_sys_config.max_texture_count = 65536;
    texture_sys_init(&p_state->textures.size, 0, tex_sys_config);
    p_state->textures.state =
        arena_allocate(&p_state->arena, p_state->textures.size);
    if (!texture_sys_init(&p_state->textures.size, p_state->textures.state,
                          tex_sys_config)) {
        ar_FATAL("Texture failed to initialize");
        return false;
    }

    /* Material System */
    material_sys_config_t mat_sys_cfg;
    mat_sys_cfg.max_material_count = 4096;
    material_sys_init(&p_state->material.size, 0, mat_sys_cfg);
    p_state->material.state =
        arena_allocate(&p_state->arena, p_state->material.size);
    if (!material_sys_init(&p_state->material.size, p_state->material.state,
                           mat_sys_cfg)) {
        ar_FATAL("Material failed to initialize");
        return false;
    }

    /* Geometry System */
    geo_sys_cfg_t geo_sys_cfg;
    geo_sys_cfg.max_geo_count = 4096;
    geometry_sys_init(&p_state->geometry.size, 0, geo_sys_cfg);
    p_state->geometry.state =
        arena_allocate(&p_state->arena, p_state->geometry.size);
    if (!geometry_sys_init(&p_state->geometry.size, p_state->geometry.state,
                           geo_sys_cfg)) {
        ar_FATAL("Geometry failed to initialize");
        return false;
    }

    // TODO: Temporary
    geo_config_t gc =
        geometry_sys_gen_plane_config(10.0f, 5.0f, 5, 5, 5.0f, 2.0f,
                                      "test geometry", "test_material");

    p_state->test_geo = geometry_sys_acquire_by_config(gc, true);

    memory_free(gc.vertices, sizeof(vertex_3d) * gc.vertex_count, MEMTAG_GAME);
	memory_free(gc.indices, sizeof(uint32_t) * gc.idx_count, MEMTAG_GAME);

	/* Load Test UI */
	geo_config_t ui_config;
	ui_config.vertex_size = sizeof(vertex_2d);
	ui_config.vertex_count = 4;
	ui_config.idx_size = sizeof(uint32_t);
	ui_config.idx_count = 6;
	string_ncopy(ui_config.material_name, "test_ui_material", MATERIAL_NAME_MAX_LENGTH);
	string_ncopy(ui_config.name, "test_ui_geometry", GEOMETRY_NAME_MAX_LENGTH);

	const float f = 512.0f;
    vertex_2d uiverts [4];

	// Top Left
    uiverts[0].position.x = 0.0f;
    uiverts[0].position.y = 0.0f;
    uiverts[0].texcoord.x = 0.0f;
    uiverts[0].texcoord.y = 0.0f;

	// Bottom Right
    uiverts[1].position.y = f;
    uiverts[1].position.x = f;
    uiverts[1].texcoord.x = 1.0f;
    uiverts[1].texcoord.y = 1.0f;

	// Bottom Left
    uiverts[2].position.x = 0.0f;
    uiverts[2].position.y = f;
    uiverts[2].texcoord.x = 0.0f;
    uiverts[2].texcoord.y = 1.0f;

	// Top Right
    uiverts[3].position.x = f;
    uiverts[3].position.y = 0.0;
    uiverts[3].texcoord.x = 1.0f;
    uiverts[3].texcoord.y = 0.0f;
    ui_config.vertices = uiverts;

    // Indices - counter-clockwise
    uint32_t uiindices[6] = {0, 1, 2, 0, 3, 1};
    //uint32_t uiindices[6] = {2, 1, 0, 3, 0, 1};
    ui_config.indices = uiindices;
	p_state->test_ui_geo = geometry_sys_acquire_by_config(ui_config, true);

	// TODO: End Temporary

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
	const b8 limit = false;

	char *stats = memory_debug_stats();
	ar_INFO(stats);
	memory_free(stats, string_length(stats) + 1, MEMTAG_STRING);

	while (p_state->is_running) {
		if (!platform_push())
			p_state->is_running = false;

		if (!p_state->is_suspend) {
			time_update(&p_state->time);
			double current_time = p_state->time.elapsed;

			float delta = (float)(current_time - p_state->last_time);
			//double delta = (current_time - p_state->last_time);
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

			// TODO: Temporary
			geo_render_data_t test_render;
			test_render.geometry = p_state->test_geo;
			test_render.model = mat4_identity();
			packet.geo_count = 1;
			packet.geometries = &test_render;

			geo_render_data_t test_ui_render;
			test_ui_render.geometry = p_state->test_ui_geo;
			test_ui_render.model = mat4_translate(vec3_zero());
			packet.ui_geo_count = 1;
			packet.ui_geometries = &test_ui_render;

			renderer_draw_frame(&packet);

			double next_frame_time = frame_start_time + target_frame_seconds;
			double frame_end_time = get_absolute_time();
			double frame_elapsed_time = frame_end_time - frame_start_time;
			runtime += frame_elapsed_time;

			if (limit && frame_end_time < next_frame_time) {
				os_sleep(next_frame_time);
                // ar_TRACE("abs sleep until %.6f (%.3fms remain)",
                // next_frame_time, (next_frame_time - frame_end_time) *
                // 1000.0);
            }
            frame_count++;

			input_update(delta);

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
	event_unreg(EVENT_CODE_DEBUG0, 0, app_on_debug);
	
	input_shut(p_state->input.state);
	geometry_sys_shut(p_state->geometry.state);
	material_sys_shut(p_state->material.state);
	texture_sys_shut(p_state->textures.state);
	renderer_shut(p_state->renderer.state);
	resource_sys_shut(p_state->resources.state);
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

