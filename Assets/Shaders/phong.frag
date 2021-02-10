#version 450
#extension GL_ARB_separate_shader_objects : enable

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

// layout(binding = 2) uniform sampler2D TexSampler;

// layout (location = 0) in vec2 TexCoord;
// layout (location = 1) in vec3 Normal;
// layout (location = 2) in vec3 Color;

// layout (location = 3) in vec3 normalVec;
// layout (location = 4) in vec3 eyeVec;
// layout (location = 5) in vec3 worldPos;


// layout(location = 0) out vec4 outColor;

layout (location = 0) in vec2 TexCoord;

layout (location = 0) out vec4 outColor;


void main() {

	// // Render-target composition

	// #define lightCount 6
	// #define ambient 0.0
	
	// // Ambient part
	// vec3 fragcolor  = vec3(ambient);
	
	// for(int i = 0; i < lightCount; ++i)
	// {
	// 	// Vector to light
	// 	vec3 L = uboComposition.lights[i].position.xyz - worldPos;
	// 	// Distance from light to fragment position
	// 	float dist = length(L);

	// 	// Viewer to fragment
	// 	vec3 V = uboComposition.viewPos.xyz - worldPos;
	// 	V = normalize(V);
		
	// 	if(dist < uboComposition.lights[i].radius)
	// 	{
	// 		// Light to fragment
	// 		L = normalize(L);

	// 		// Attenuation
	// 		float atten = uboComposition.lights[i].radius / (pow(dist, 2.0) + 1.0);

	// 		// Diffuse part
	// 		vec3 N = normalize(normalVec);
	// 		float NdotL = max(0.0, dot(N, L));
	// 		vec3 diff = uboComposition.lights[i].color * Color.rgb * NdotL * atten;

	// 		// Specular part
	// 		// Specular map values are stored in alpha of albedo mrt
	// 		vec3 R = reflect(-L, N);
	// 		float NdotR = max(0.0, dot(R, V));
	// 		vec3 spec = uboComposition.lights[i].color * pow(NdotR, 16.0) * atten;

	// 		fragcolor += diff + spec;	
	// 	}	
	// }   

    outColor = vec4(0.5, 0.3, 0.7, 1.0f);
}