#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) flat in int Colliding;

layout (location = 0) out vec4 outColor;



void main() {
	outColor = vec4((Colliding == 1) ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0), 1.0);
}