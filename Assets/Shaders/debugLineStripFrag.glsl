#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstant
{
	layout(offset = 64) vec4 color;
} push;


layout (location = 0) out vec4 outColor;



void main() {
	outColor = vec4(push.color.rgb, 1.0);
}