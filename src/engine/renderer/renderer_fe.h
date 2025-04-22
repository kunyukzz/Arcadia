#ifndef __RENDERER_FRONTEND_H__
#define __RENDERER_FRONTEND_H__

#include "engine/define.h"
#include "engine/renderer/renderer_type.h"

b8 renderer_init(uint64_t *memory_require, void *state, const char *name);
void renderer_shut(void *state);
void renderer_resize(uint32_t width, uint32_t height);
b8 renderer_draw_frame(render_packet_t *packet);

#endif //__RENDERER_FRONTEND_H__
