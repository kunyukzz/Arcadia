#include "engine/renderer/renderer_be.h"
#include "engine/renderer/vulkan/vk_backend.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"

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

		backend->begin_renderpass = vk_backend_begin_renderpass;
		backend->end_renderpass = vk_backend_end_renderpass;

		backend->init_tex = vk_backend_tex_init;
		backend->shut_tex = vk_backend_tex_shut;

		backend->draw_geometry = vk_backend_geo_render;
		backend->init_geo = vk_backend_geometry_init;
		backend->shut_geo = vk_backend_geometry_shut;

		backend->create_shader = vk_backend_shader_create;
		backend->shut_shader = vk_backend_shader_shut;
		backend->set_uniform_shader = vk_backend_set_uniform;
		backend->init_shader = vk_backend_shader_init;
		backend->use_shader = vk_backend_shader_use;
		backend->bind_shader_global = vk_backend_shader_bind_globals;
		backend->bind_shader_instance = vk_backend_shader_bind_instance;
		backend->apply_shader_global = vk_backend_shader_apply_globals;
		backend->apply_shader_instance = vk_backend_shader_apply_instance;
		backend->acquire_shader_inst_resc = vk_backend_shader_acquire_inst_resc;
		backend->release_shader_inst_resc = vk_backend_shader_release_inst_resc;

		return true;
	}
	return false;
}

void renderer_be_shut(render_backend_t *backend) {
	if (!backend) {
        ar_WARNING("renderer_be_shut: backend is NULL!");
        return;
    }

	memory_zero(backend, sizeof(render_backend_t));
}
