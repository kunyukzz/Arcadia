#include "engine/renderer/renderer_be.h"
#include "engine/renderer/vulkan/vk_backend.h"
#include "engine/core/logger.h"

b8 renderer_be_init(render_backend_type_t type, render_backend_t *backend) {
	if (type == BACKEND_OPENGL) {
		return false;
	}

	if (type == BACKEND_VULKAN) {
		backend->init = vk_backend_init;
		backend->begin_frame = vk_backend_begin_frame;
		backend->end_frame = vk_backend_end_frame;
		backend->resize = vk_backend_resize;
		backend->shut = vk_backend_shut;

		backend->update_world = vk_backend_update_world;
		backend->update_ui = vk_backend_update_ui;

		backend->begin_renderpass = vk_backend_begin_renderpass;
		backend->end_renderpass = vk_backend_end_renderpass;

		backend->init_tex = vk_backend_tex_init;
		backend->shut_tex = vk_backend_tex_shut;

		backend->init_material = vk_backend_material_init;
		backend->shut_material = vk_backend_material_shut;

		backend->draw_geometry = vk_backend_geo_render;
		backend->init_geo = vk_backend_geometry_init;
		backend->shut_geo = vk_backend_geometry_shut;
		return true;
	}
	return false;
}

void renderer_be_shut(render_backend_t *backend) {
	if (!backend) {
        ar_WARNING("renderer_be_shut: backend is NULL!");
        return;
    }

	backend->init = 0;
	backend->begin_frame = 0;
	backend->end_frame = 0;
	backend->resize = 0;
	backend->shut = 0;

	backend->update_world = 0;
	backend->update_ui = 0;

	backend->begin_renderpass = 0;
	backend->end_renderpass = 0;

	backend->init_tex = 0;
	backend->shut_tex = 0;

	backend->init_material = 0;
	backend->shut_material = 0;

	backend->draw_geometry = 0;
	backend->init_geo = 0;
	backend->shut_geo = 0;
}
