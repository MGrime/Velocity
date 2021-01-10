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
}pc;

layout(binding = 1) uniform PointLightData {
	uint			Count;
	PointLight[128]	Lights;
} pointLights;

layout(binding = 16) uniform sampler2D texSampler[128];

void main() {
	// Normalize incoming data
	vec3 norm = normalize(fragNormal);

	// Calculate direction from camera to fragment
	vec3 cameraDirection = normalize(vec3(pc.cameraPosX,pc.cameraPosY,pc.cameraPosZ) - fragPosition);

	vec3 totalDiffuse = vec3(0.0f);
	vec3 totalSpecular = vec3(0.0f);

	for (uint i = 0; i < pointLights.Count; ++i)
	{
		vec3 lightColor = pointLights.Lights[i].Color * 10.0f;
		vec3 lightDirection = normalize(pointLights.Lights[i].Position - fragPosition);
		float lightLength = length(fragPosition - pointLights.Lights[i].Position);
		
		vec3 diffuseLight = (lightColor * max(dot(fragNormal, lightDirection),0)/ lightLength);

		vec3 halfway = normalize(lightDirection + cameraDirection);

		vec3 specularLight = diffuseLight * pow(max(dot(fragNormal,halfway),0.0f),64.0f);

		totalDiffuse += diffuseLight;
		totalSpecular += specularLight;


	}

	vec4 textureColor = texture(texSampler[pc.texIndex],fragUV);

	vec3 diffuseMat = textureColor.rgb;
	float specMat = textureColor.a;

	vec3 finalColor = (totalDiffuse * diffuseMat) + (totalSpecular *specMat);
	
	outColor = vec4(finalColor,1.0f);

}
