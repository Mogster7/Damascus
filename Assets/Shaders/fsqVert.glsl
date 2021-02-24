#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec2 TexCoord;
// layout (location = 1) out vec3 Normal;
// layout (location = 2) out vec3 Color;

// layout (binding = 0) uniform UboViewProjection
// {
//     mat4 projection;
//     mat4 view;
// } uboViewProjection;

// struct Light
// {
//     vec4 position;
//     vec3 color;
//     float radius;
// };

// layout (binding = 1) uniform UboComposition
// {
//     Light lights[6];
//     vec4 viewPos;
//     int debugDisplayTarget;
// } uboComposition;

// layout(push_constant) uniform PushModel
// {
//     mat4 model;
// } pushModel;

// layout (location = 3) out vec3 normalVec;
// layout (location = 4) out vec3 eyeVec;
// layout (location = 5) out vec3 worldPos;


void main() 
{
    gl_Position = vec4(vertPos, 1.0);
    TexCoord = texCoord;
}