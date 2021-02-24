#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 TexCoord;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec3 Color;
layout (location = 3) in vec3 normalVec;
layout (location = 4) in vec3 worldPos;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;

void main() {
	outPosition = vec4(worldPos, 1.0);
	outNormal = vec4(Normal, 1.0);
	outAlbedo = vec4(Color, 1.0);
}