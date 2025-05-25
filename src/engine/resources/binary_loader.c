#include "engine/resources/binary_loader.h"

#include "engine/core/ar_strings.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"
#include "engine/platform/filesystem.h"
#include "engine/resources/resc_type.h"
#include "engine/systems/resource_sys.h"

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
b8 binary_loader_load(resource_loader_t *self, const char *name, resource_t *resc) {
	if (!self || !name || !resc) return false;

	char *format_str = "%s/%s/%s%s";
	char full_path[512];
    string_format(full_path, format_str, resource_sys_base_path(), self->type_path, name, "");

	// TODO: Should use allocator.
	resc->full_path = string_duplicate(full_path);
	ar_TRACE("Path: %s", resc->full_path);

	file_handle_t f;
	if (!filesystem_open(full_path, MODE_READ, false, &f)) {
		ar_ERROR("binary_loader_load - unable to open file: '%s'", full_path);
		return false;
	}

	uint64_t file_size = 0;
	if (!filesystem_size(&f, &file_size)) {
		ar_ERROR("unknown size of text file: %s", full_path);
		filesystem_close(&f);
		return false;
	}

	// TODO: Should use allocator.
	uint8_t *resc_data = memory_alloc(sizeof(uint8_t) * file_size, MEMTAG_ARRAY);
	uint64_t read_size = 0;
	if (!filesystem_read_all_byte(&f, resc_data, &read_size)) {
		ar_ERROR("unable to read binary file: %s", full_path);
		filesystem_close(&f);
		return false;
	}
	filesystem_close(&f);

	resc->data = resc_data;
	resc->data_size = read_size;
	resc->name = name;

	return true;
}

void binary_loader_unload(resource_loader_t *self, resource_t *resc) {
	 if (!self || !resc) {
        ar_WARNING("binary_loader_unload - call with nullptr.");
        return;
    }

    uint32_t path_length = string_length(resc->full_path);
    if (path_length)
        memory_free(resc->full_path, sizeof(char) * path_length + 1,
                    MEMTAG_STRING);

    if (resc->data) {
        memory_free(resc->data, resc->data_size, MEMTAG_ARRAY);
        resc->data      = 0;
        resc->data_size = 0;
        resc->id_loader = INVALID_ID;
    }

}
/* ========================================================================== */
/* ========================================================================== */

resource_loader_t loader_binary_rsc_init(void) {
	resource_loader_t loader;
	loader.type = RESC_TYPE_BINARY;
	loader.custom_type = 0;
	loader.load = binary_loader_load;
	loader.unload = binary_loader_unload;
	loader.type_path = "";

	return loader;
}
