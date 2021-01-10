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
	vec3 normal = normalize(transpose(inverse(mat3(pc.model))) * fragNormal);

	// Precalculate
	vec3 surfacePos = vec3(pc.model * vec4(fragPosition, 1.0f));

	vec3 cameraPos = vec3(pc.cameraPosX,pc.cameraPosY,pc.cameraPosZ);
	vec3 cameraDirection = normalize(cameraPos - surfacePos);

	// Handle point Lights
	vec3 totalDiffuse = vec3(0.0f,0.0f,0.0f);
	vec3 totalSpecular = vec3(0.0f,0.0f,0.0f);

	for (uint i = 0; i < pointLights.Count; ++i)
	{
		// Precalc
		vec3 lightDirection = normalize(pointLights.Lights[i].Position - surfacePos);
		float lightLength = length(surfacePos - pointLights.Lights[i].Position);

		// Get diffuse
		vec3 diffuseLight = (pointLights.Lights[i].Color * max(dot(normal,lightDirection),0) / lightLength);

		// Get specular
		vec3 halfway = normalize(lightDirection + cameraDirection);
		vec3 specularLight = diffuseLight * pow(max(dot(normal,halfway),0),32.0f);

		// Sum
		totalDiffuse += diffuseLight;
		totalSpecular += specularLight;
	}

	vec3 textureColor = texture(texSampler[pc.texIndex],fragUV).rgb;

	vec3 ambient = vec3(0.2f,0.2f,0.2f);
	vec3 finalColor = ((ambient + totalDiffuse) * textureColor) + (totalSpecular);
	
	outColor = vec4(finalColor,1.0);
	
}