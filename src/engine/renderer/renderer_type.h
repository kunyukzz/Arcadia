#ifndef __RENDERER_TYPE_H__
#define __RENDERER_TYPE_H__

#include "engine/define.h"
#include "engine/math/math_type.h"
#include "engine/resources/resc_type.h"

#define BUILTIN_SHADER_NAME_MATERIAL "Builtin.MaterialShader"
#define BUILTIN_SHADER_NAME_UI "Builtin.UIShader"

struct shader_t;
struct shader_uniform_t;

typedef enum render_backend_type_t {
	BACKEND_OPENGL,
	BACKEND_VULKAN
} render_backend_type_t;

typedef struct geo_render_data_t {
	mat4 model;
	geometry_t *geometry;
} geo_render_data_t;

typedef enum builtin_renderpass_t {
	RENDER_LAYER_WORLD = 0x01,
	RENDER_LAYER_UI = 0x02
} builtin_renderpass_t;

typedef struct render_backend_t {
	uint64_t frame_number;
	texture_t *default_diffuse;

	b8 (*init)(struct render_backend_t *backend, const char *name);
	void (*shut)(struct render_backend_t *backend);
    void (*resize)(struct render_backend_t *backend, uint32_t width,
                   uint32_t height);

    b8 (*begin_frame)(struct render_backend_t *backend, float delta_time);
    b8 (*end_frame)(struct render_backend_t *backend, float delta_time);

	b8 (*begin_renderpass)(struct render_backend_t *backend, uint8_t renderpass_id);
	b8 (*end_renderpass)(struct render_backend_t *backend, uint8_t renderpass_id);

    void (*init_tex)(const uint8_t *pixel, texture_t *texture);
    void (*shut_tex)(texture_t *texture);

	b8 (*init_material)(material_t *material);
	void (*shut_material)(material_t *material);

    void (*draw_geometry)(geo_render_data_t data);

    b8 (*init_geo)(geometry_t *geometry, uint32_t vertex_size,
                   uint32_t vertex_count, const void *vertices,
                   uint32_t idx_size, uint32_t idx_count,
                   const void *indices);

    void (*shut_geo)(geometry_t *geometry);

    b8 (*create_shader)(struct shader_t *shader, uint8_t renderpass_id,
                      uint8_t stage_count, const char **stage_filenames,
                      shader_stage_t *stages);

	void (*shut_shader)(struct shader_t *shader);

	b8 (*init_shader)(struct shader_t *shader);
	b8 (*use_shader)(struct shader_t *shader);
	b8 (*bind_shader_global)(struct shader_t *shader);
	b8 (*bind_shader_instance)(struct shader_t *shader, uint32_t instance_id);
	b8 (*apply_shader_global)(struct shader_t *shader);
	b8 (*apply_shader_instance)(struct shader_t *shader);
	b8 (*acquire_shader_inst_resc)(struct shader_t *shader, uint32_t *instance_id);
	b8 (*release_shader_inst_resc)(struct shader_t *shader, uint32_t instance_id);
	b8 (*set_uniform_shader)(struct shader_t *fe_shader, struct shader_uniform_t *uniform, const void *value);
} render_backend_t;

typedef struct render_packet_t {
	float delta;
	uint32_t geo_count;
	uint32_t ui_geo_count;

	geo_render_data_t *geometries;
	geo_render_data_t *ui_geometries;
} render_packet_t;

#endif //__RENDERER_TYPE_H__
