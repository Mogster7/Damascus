#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(vec3(0.88, 0.97, 0.98), 1.0);
}