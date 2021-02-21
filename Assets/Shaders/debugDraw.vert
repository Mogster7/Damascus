#version 450

layout (location = 0) in vec3 vertPos;

layout (binding = 0) uniform UboViewProjection
{
    mat4 projection;
    mat4 view;
} uboViewProjection;

layout(push_constant) uniform PushModel
{
    mat4 model;
} pushModel;


void main() {

    gl_Position = uboViewProjection.projection * uboViewProjection.view * pushModel.model * vec4(vertPos, 1.0);
}