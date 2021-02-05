#version 450

layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec3 vertColor;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out vec2 TexCoord;

layout (binding = 0) uniform UboViewProjection
{
    mat4 projection;
    mat4 view;
} uboViewProjection;

//layout (binding = 1) uniform UboModel
//{
//    mat4 model;
//} uboModel;

layout(push_constant) uniform PushModel
{
    mat4 model;
} pushModel;


void main() {
    gl_Position = uboViewProjection.projection * uboViewProjection.view * pushModel.model * vec4(vertPos, 1.0);
    TexCoord = texCoord;
}