#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec3 vertPos;
layout(location = 0) out flat int Colliding;

layout (binding = 0) uniform UboViewProjection
{
    mat4 projection;
    mat4 view;
} uboViewProjection;

layout(push_constant) uniform PushConstant
{
    mat4 model;
	int colliding;
} push;


void main() {

    gl_Position = uboViewProjection.projection * uboViewProjection.view * push.model * vec4(vertPos, 1.0);
    Colliding = push.colliding ? 1 : 0;
}