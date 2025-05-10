#ifndef __RENDERER_FRONTEND_H__
#define __RENDERER_FRONTEND_H__

#include "engine/define.h"
#include "engine/renderer/renderer_type.h"

b8 renderer_init(uint64_t *memory_require, void *state, const char *name);
void renderer_shut(void *state);
void renderer_resize(uint32_t width, uint32_t height);
b8 renderer_draw_frame(render_packet_t *packet);
void renderer_set_view(mat4 view);

void renderer_tex_init(const char *name, int32_t width,
                       int32_t height, int32_t channel_count,
                       const uint8_t *pixel, b8 has_transparent,
                       texture_t *texture);
void renderer_tex_shut(texture_t *texture);

#endif //__RENDERER_FRONTEND_H__
