#ifndef __RENDERER_FRONTEND_H__
#define __RENDERER_FRONTEND_H__

#include "engine/define.h"
#include "engine/renderer/renderer_type.h"

b8 renderer_init(uint64_t *memory_require, void *state, const char *name);
void renderer_shut(void *state);
void renderer_resize(uint32_t width, uint32_t height);
b8 renderer_draw_frame(render_packet_t *packet);
void renderer_set_view(mat4 view);

void renderer_tex_init(const uint8_t *pixel, texture_t *texture);
void renderer_tex_shut(texture_t *texture);

b8 renderer_material_init(material_t *material);
void renderer_material_shut(material_t *material);

b8   renderer_geometry_init(geometry_t *geometry, uint32_t vertex_size,
                            uint32_t vertex_count, const void *vertices,
                            uint32_t idx_size, uint32_t idx_count,
                            const void *indices);

void renderer_geometry_shut(geometry_t *geometry);

#endif //__RENDERER_FRONTEND_H__
