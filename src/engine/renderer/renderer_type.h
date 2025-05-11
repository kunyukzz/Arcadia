#ifndef __RENDERER_TYPE_H__
#define __RENDERER_TYPE_H__

#include "engine/define.h"
#include "engine/math/math_type.h"
#include "engine/resources/resc_type.h"

typedef enum render_backend_type_t {
	BACKEND_OPENGL,
	BACKEND_VULKAN
} render_backend_type_t;

typedef struct global_uni_obj_t {
	mat4 projection;
	mat4 viewx;
	mat4 _reserved0;
	mat4 _reserved1;
} global_uni_obj_t;

typedef struct material_uni_obj_t {
	vec4 diffuse_col;
	vec4 v_reserved0;
	vec4 v_reserved1;
	vec4 v_reserved2;
} material_uni_obj_t;

typedef struct geo_render_data_t {
	mat4 model;
	material_t *material;
} geo_render_data_t;

typedef struct render_backend_t {
	uint64_t frame_number;
	texture_t *default_diffuse;

	b8 (*init)(struct render_backend_t *backend, const char *name);
	void (*shut)(struct render_backend_t *backend);
    void (*resize)(struct render_backend_t *backend, uint32_t width,
                   uint32_t height);

    b8 (*begin_frame)(struct render_backend_t *backend, float delta_time);
    void (*update_global)(mat4 projection, mat4 viewx, vec3 view_pos,
                          vec4 ambient_color, int32_t mode);
    b8 (*end_frame)(struct render_backend_t *backend, float delta_time);
	void (*update_obj)(geo_render_data_t data);

    void (*init_tex)(const uint8_t *pixel, texture_t *texture);
    void (*shut_tex)(texture_t *texture);

	b8 (*init_material)(material_t *material);
	void (*shut_material)(material_t *material);

} render_backend_t;

typedef struct render_packet_t {
	float delta;
} render_packet_t;

#endif //__RENDERER_TYPE_H__
