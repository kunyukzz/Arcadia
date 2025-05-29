#ifndef __LOADER_UTILS_H__
#define __LOADER_UTILS_H__

#include "engine/define.h"
#include "engine/memory/memory.h"
#include "engine/resources/resc_type.h"

struct resource_loader_t;

b8 resc_unload(struct resource_loader_t *self, resource_t *resource,
               mem_tag_t mem_tag);

#endif //__LOADER_UTILS_H__
