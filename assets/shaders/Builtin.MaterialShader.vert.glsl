// Vertex Shaders

#version 420
//#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_texcoord;

layout(set = 0, binding = 0) uniform global_uni_obj {
	mat4 projection;
	mat4 view;
} global_ubo;

layout(push_constant) uniform push_constants {
	mat4 model;
} u_push_consts;

//layout(location = 0) out int out_mode;

// Data Transfer Object
layout(location = 1) out struct dto {
	vec2 tex_coord;
} out_dto;

void main() {
	out_dto.tex_coord = in_texcoord;
	gl_Position = global_ubo.projection * global_ubo.view * u_push_consts.model
		* vec4(in_pos, 1.0);
}

