#include "engine/resources/loader_utils.h"

#include "engine/core/logger.h"
#include "engine/core/ar_strings.h"

b8 resc_unload(struct resource_loader_t *self, resource_t *resource,
               mem_tag_t mem_tag) {
    if (!self || !resource) {
        ar_WARNING("resc_unload - called with nullptr");
        return false;
    }

    uint32_t path_length = string_length(resource->full_path);
    if (path_length) {
        memory_free(resource->full_path, sizeof(char) * path_length + 1,
                    MEMTAG_STRING);
    }

    if (resource->data) {
        memory_free(resource->data, resource->data_size, mem_tag);
        resource->data      = 0;
        resource->data_size = 0;
        resource->id_loader = INVALID_ID;
    }

    return true;
}
