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
		backend->update_global = vk_backend_update_global;
		backend->end_frame = vk_backend_end_frame;
		backend->resize = vk_backend_resize;
		backend->shut = vk_backend_shut;
		backend->update_obj = vk_backend_update_obj;
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
	backend->update_global = 0;
	backend->end_frame = 0;
	backend->resize = 0;
	backend->shut = 0;
	backend->update_obj = 0;
}
