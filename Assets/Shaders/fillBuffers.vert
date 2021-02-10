#version 450

layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec3 vertNormal;
layout (location = 2) in vec3 vertColor;
layout (location = 3) in vec2 texCoord;

layout (location = 0) out vec2 TexCoord;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec3 Color;
layout (location = 3) out vec3 normalVec;
layout (location = 4) out vec3 worldPos;

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

    vec3 worldPos = (pushModel.model * vec4(vertPos, 1.0)).xyz;
    normalVec = (pushModel.model * vec4(vertNormal, 0.0)).xyz;

    Normal = vertNormal;
    TexCoord = texCoord;
    Color = vertColor;
}