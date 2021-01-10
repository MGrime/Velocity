#version 450

struct PointLight
{
	vec3 Position;
	vec3 Color;
};

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 model;
	layout(offset = 64) uint texIndex;
	layout(offset = 68) float cameraPosX;
	layout(offset = 72) float cameraPosY;
	layout(offset = 76) float cameraPosZ;
} pc;

layout(binding = 1) uniform PointLightData {
	uint			Count;
	PointLight[128]	Lights;
} pointLights;

layout(binding = 16) uniform sampler2D texSampler[128];

void main() {

	// Normalise incoming
	vec3 norm = normalize(fragNormal);

	// Ambient as a fixed amount
	vec3 ambient = vec3(0.2f,0.2f,0.2f);
	
	// Loop Lights
	vec3 totalDiff = vec3(0.0f);
	vec3 totalSpec = vec3(0.0f);

	for (uint i = 0; i < pointLights.Count; ++i)
	{
		PointLight light = pointLights.Lights[i];

		vec3 lightDirection = normalize(light.Position - fragPosition);
		float diff = max(dot(norm,lightDirection),0.0f);
		vec3 diffuse = light.Color * diff;

		vec3 cameraPos = vec3(pc.cameraPosX, pc.cameraPosY, pc.cameraPosZ);
		vec3 viewDir = normalize(cameraPos - fragPosition);
		vec3 reflectDir = reflect(-lightDirection, norm);
		float spec = pow(max(dot(viewDir, reflectDir),0.0f),32.0f);
		vec3 specular = light.Color * spec;

		float distance = length(light.Position - fragPosition);
		float attenuation = 1.0f / (1.0f + 0.09f * distance + 0.032f * (distance * distance));

		diffuse *= attenuation;
		specular *= attenuation;
		
		totalDiff += diffuse;
		totalSpec += specular;
	}

	vec4 textureColor = texture(texSampler[pc.texIndex],fragUV);

	outColor = vec4((ambient + totalDiff + totalSpec) * textureColor.rgb,textureColor.a);

}