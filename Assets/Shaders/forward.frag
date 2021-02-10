#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 TexCoord;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec3 Color;
layout (location = 3) in vec3 normalVec;
layout (location = 4) in vec3 worldPos;

layout (binding = 5) uniform sampler2D texSampler;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(vec3(0.3, 0.6, 0.8), 1.0);
}