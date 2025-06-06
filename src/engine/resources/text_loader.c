#include "engine/resources/text_loader.h"

#include "engine/core/ar_strings.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"
#include "engine/platform/filesystem.h"
#include "engine/resources/resc_type.h"
#include "engine/resources/loader_utils.h"
#include "engine/systems/resource_sys.h"


/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
b8 text_loader_load(resource_loader_t *self, const char *name, resource_t *resc) {
	if (!self || !name || !resc) return false;

	char *format_str = "%s/%s/%s%s";
	char full_path[512];
    string_format(full_path, format_str, resource_sys_base_path(),
                  self->type_path, name, "");

	file_handle_t f;
	if (!filesystem_open(full_path, MODE_READ, false, &f)) {
		ar_ERROR("text_loader_load - unable to open file: '%s'", full_path);
		return false;
	}

    // TODO: Should use allocator.
	resc->full_path = string_duplicate(full_path);

	uint64_t file_size = 0;
	if (!filesystem_size(&f, &file_size)) {
		ar_ERROR("unknown size of text file: %s", full_path);
		filesystem_close(&f);
		return false;
	}

	// TODO: Should use allocator.
	char *resc_data = memory_alloc(sizeof(char) * file_size, MEMTAG_ARRAY);
	uint64_t read_size = 0;
	if (!filesystem_read_all_text(&f, resc_data, &read_size)) {
		ar_ERROR("unable to text read text file: %s", full_path);
		filesystem_close(&f);
		return false;
	}
	filesystem_close(&f);

	resc->data = resc_data;
	resc->data_size = read_size;
	resc->name = name;

	return true;
}

void text_loader_unload(resource_loader_t *self, resource_t *resc) {
    if (!resc_unload(self, resc, MEMTAG_TEXTURE)) {
        ar_WARNING("text_loader_unload - call with nullptr.");
    }
}
/* ========================================================================== */
/* ========================================================================== */

resource_loader_t loader_text_rsc_init(void) {
	resource_loader_t loader;
	loader.type = RESC_TYPE_TEXT;
	loader.custom_type = 0;
	loader.load = text_loader_load;
	loader.unload = text_loader_unload;
	loader.type_path = "";

	return loader;
}
