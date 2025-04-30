// Vertex Shaders

#version 420
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_pos;
layout(location = 0) out vec3 out_pos;

layout(set = 0, binding = 0) uniform global_uni_obj {
	mat4 projection;
	mat4 view;
} global_ubo;

void main() {
	gl_Position = global_ubo.projection * global_ubo.view * vec4(in_pos, 1.0);
	out_pos = in_pos;
}

