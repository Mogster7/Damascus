#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

struct Light
{
    vec4 position;
    vec3 color;
    float radius;
};

layout (location = 0) in vec2 TexCoord;

layout (binding = 1) uniform sampler2D samplerPosition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;
layout (binding = 4) uniform UboComposition
{
    Light lights[6];
    vec4 viewPos;
	float globalLightStrength;
    int debugDisplayTarget;
} uboComposition;

layout (location = 0) out vec4 outColor;


void main() {

	// Get G-Buffer values
	vec3 worldPos = texture(samplerPosition, TexCoord).rgb;
	vec3 normal = texture(samplerNormal, TexCoord).rgb;
	vec4 albedo = texture(samplerAlbedo, TexCoord);

		// Debug display
	if (uboComposition.debugDisplayTarget > 0) {
		switch (uboComposition.debugDisplayTarget) {
			case 1: 
				outColor.rgb = worldPos;
				break;
			case 2: 
				outColor.rgb = normal;
				break;
			case 3: 
				outColor.rgb = albedo.rgb;
				break;
			case 4: 
				outColor.rgb = albedo.aaa;
				break;
		}		
		outColor.a = 1.0;
		return;
	}

	// // Render-target composition

	#define lightCount 6
	#define ambient 1.0
	
	// Ambient part
	vec3 fragcolor  = vec3(ambient) * albedo.rgb;
	
	for(int i = 0; i < lightCount; ++i)
	{
		// Vector to light
		vec3 L = uboComposition.lights[i].position.xyz - worldPos;
		// Distance from light to fragment position
		float dist = length(L);

		// Viewer to fragment
		vec3 V = uboComposition.viewPos.xyz - worldPos;
		V = normalize(V);
		
		if(dist < uboComposition.lights[i].radius)
		{
			// Light to fragment
			L = normalize(L);

			// Attenuation
			float atten = uboComposition.lights[i].radius / (pow(dist, 2.0) + 1.0);

			// Diffuse part
			vec3 N = normalize(normal);
			float NdotL = max(0.0, dot(N, L));
			vec3 diff = uboComposition.lights[i].color * albedo.rgb * NdotL * atten;

			// Specular part
			// Specular map values are stored in alpha of albedo mrt
			vec3 R = reflect(-L, N);
			float NdotR = max(0.0, dot(R, V));
			vec3 spec = uboComposition.lights[i].color * pow(NdotR, 2.0) * atten;

			fragcolor += (diff + spec) * uboComposition.globalLightStrength;	
		}	
	}   

    outColor = vec4(fragcolor, 1.0);
}