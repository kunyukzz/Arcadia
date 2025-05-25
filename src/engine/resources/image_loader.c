#include "engine/resources/image_loader.h"

#include "engine/core/ar_strings.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"
#include "engine/resources/resc_type.h"
#include "engine/systems/resource_sys.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wcast-align"

#define STB_IMAGE_IMPLEMENTATION
#include "engine/include/stb_image.h"
#pragma clang diagnostic pop

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
b8 image_loader_load(resource_loader_t *self, const char *name,
                     resource_t *resc) {
    if (!self || !name || !resc) return false;

    char *format_str = "%s/%s/%s%s";
	const int32_t req_channel_count = 4;
	stbi_set_flip_vertically_on_load(true);
	char full_path[512];
    string_format(full_path, format_str, resource_sys_base_path(),
                  self->type_path, name, ".png");

	int32_t width, height, channel_count;

	// Assume 8 bits per channel. 4 channel
	// TODO: Make configureable
    uint8_t *data    = stbi_load(full_path, &width, &height, &channel_count,
                                 req_channel_count);

    /* failure check */
	const char *fail = stbi_failure_reason();
	if (fail) {
		ar_ERROR("failed to load: %s:'%s'", full_path, fail);
		stbi__err(0, 0);

		if (data) stbi_image_free(data);
		return false;
	}

	if (!data) {
		ar_ERROR("failed to load file %s", full_path);
		return false;
	}

    // TODO: Should use allocator.
	resc->full_path = string_duplicate(full_path);

    image_resc_data_t *resc_data =
        memory_alloc(sizeof(image_resc_data_t), MEMTAG_TEXTURE);
	resc_data->pixels = data;
	resc_data->width = (uint32_t)width;
	resc_data->height = (uint32_t)height;
	resc_data->channel_count = req_channel_count;

	resc->data = resc_data;
	resc->data_size = sizeof(image_resc_data_t);
	resc->name = name;

    return true;
}

void image_loader_unload(resource_loader_t *self, resource_t *resc) {
	if (!self || !resc) {
		ar_WARNING("image_loader_unload - call with nullptr");
		return;
	}

	uint32_t path_length = string_length(resc->full_path);
	if (path_length) {
        memory_free(resc->full_path, sizeof(char) * path_length + 1,
                    MEMTAG_STRING);
		resc->full_path = 0;
	}

    if (resc->data) {
		image_resc_data_t *resc_data = (image_resc_data_t *)resc->data;

		if (resc_data->pixels) {
			stbi_image_free(resc_data->pixels);
			resc_data->pixels = 0;
		}

		memory_free(resc->data, resc->data_size, MEMTAG_TEXTURE);
		resc->data = 0;
		resc->data_size = 0;
		resc->id_loader = INVALID_ID;
	}
}
/* ========================================================================== */
/* ========================================================================== */

resource_loader_t loader_image_rsc_init(void) {
	resource_loader_t loader;
	loader.type = RESC_TYPE_IMAGE;
	loader.custom_type = 0;
	loader.load = image_loader_load;
	loader.unload = image_loader_unload;
	loader.type_path = "textures";

	return loader;
}
