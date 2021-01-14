#version 450

struct PointLight
{
	vec3 Position;
	vec3 Color;
};

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;		// Model space
layout(location = 2) in vec3 fragTangent;		// Model space
layout(location = 3) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 model;
	layout(offset = 64) int albedoIndex;
	layout(offset = 68) int normalIndex;
	layout(offset = 72) int heightIndex;
	layout(offset = 76) int metallicIndex;
	layout(offset = 80) int roughnessIndex;
	layout(offset = 84) float cameraPosX;
	layout(offset = 88) float cameraPosY;
	layout(offset = 92) float cameraPosZ;
} pc;

layout(binding = 1) uniform PointLightData {
	uint			Count;
	PointLight[128]	Lights;
} pointLights;

layout(binding = 16) uniform sampler2D texSampler[128];

const float PI = 3.1415f;

// Gets normal applying parallax
vec3 getNormalFromMap(inout vec2 UV)
{
	vec3 modelNormal = normalize(fragNormal);
	vec3 modelTangent = normalize(fragTangent);

	// Calculate inverse tangent matrix in model space
	vec3 biTangent = cross(modelNormal, modelTangent);					// Model space
	mat3 invTangentMatrix = mat3(modelTangent, biTangent, modelNormal);	// Model space

	if (pc.heightIndex != -1)
	{
		// Calculate camera direction
		vec3 cameraPos = vec3(pc.cameraPosX,pc.cameraPosY,pc.cameraPosZ);
		vec3 cameraDir = normalize(cameraPos - fragPosition);
		// TODO: CHECK
		mat3 invWorldMatrix = mat3(pc.model);
		vec3 cameraModelDir = normalize(cameraDir * invWorldMatrix);
	
		// Calculate UV offset
		// TODO: CHECK
		mat3 tangentMatrix = invTangentMatrix;
		vec2 offsetDir = (cameraModelDir * tangentMatrix).xy;
	
		// offset uvec2
		float texDepth = 0.06f * (texture(texSampler[pc.heightIndex],UV).r - 0.5f);
		UV += texDepth * offsetDir;
	}

	// Extract normal from map and shift to -1 to 1 range
	vec3 textureNormal = 2.0f * texture(texSampler[pc.normalIndex],UV).rgb - 1.0f;
	textureNormal.y = -textureNormal.y;

	// Convert normal from tangent to world space
	// TODO: CHECK THIS
	return normalize((textureNormal * invTangentMatrix) * mat3(pc.model));
}

// Normal distribution function
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N,H),0.0f);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0);
	denom = PI * denom * denom;
	
	return nom / denom;
}

// Geometry function sub part
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0f);
	float k = (r * r) / 8.0f;

	float nom = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom / denom;
}

// Geometry function
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N,V),0.0f);
	float NdotL = max(dot(N,L),0.0f);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

// Reflectance
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0f - F0) * pow(max(1.0f - cosTheta,0.0f),5.0f);
}

void main() {
	// Sample normal first to calculate offset UV
	vec2 offsetUV = fragUV;

	// World space
	vec3 N = getNormalFromMap(offsetUV);	

	// World space
	vec3 cameraPos = vec3(pc.cameraPosX,pc.cameraPosY,pc.cameraPosZ);
	vec3 V = normalize(cameraPos - fragPosition);	

	// Sample textures
	// Albedo color normalised to linear space
	vec3 albedo = pow(texture(texSampler[pc.albedoIndex],offsetUV).rgb,vec3(2.2f));

	// Roughness and shinyness in linear space
	float metallic = texture(texSampler[pc.metallicIndex],offsetUV).r;
	float roughness = texture(texSampler[pc.roughnessIndex],offsetUV).r;
	// TODO: AO

	// Calculate reflectance at normal incidence.
	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedo, metallic);

	// Reflence equation
	vec3 Lo = vec3(0.0f);
	for (uint i = 0; i < pointLights.Count; ++i)
	{
		PointLight light = pointLights.Lights[i];

		// Covert color into higher space
		light.Color = light.Color * 255.0f;
		
		// Calculate radiance for light 
		vec3 L = normalize(light.Position - fragPosition);
		vec3 H = normalize(V + L);
		float lightLength = length(light.Position - fragPosition);
		float attenuation = 1.0f / (lightLength * lightLength);
		vec3 radiance = light.Color * attenuation;

		// Cook-Torrance BRDF
		float NDF = DistributionGGX(N,H,roughness);
		float G = GeometrySmith(N,V,L,roughness);
		vec3 F = fresnelSchlick(max(dot(H,V),0.0f),F0);

		// kS == fresnel
		vec3 kS = F;
		
		// Energy conservation
		vec3 kD = vec3(1.0f) - kS;
		kD *= 1.0f - metallic;

		vec3 nominator = NDF * G * F;
		float denominator = 4 * max(dot(N,V),0.0f) * max(dot(N,L),0.0f);
		vec3 specular = nominator / max(denominator,0.001);

		// Scale light
		float NdotL = max(dot(N,L),0.0f);

		// Add to outgoing
		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	// Calculate ambient
	vec3 ambient = vec3(0.03) * albedo;

	// Final color in linear space
	vec3 color = ambient + Lo;

	// Tonemap
	color = color / (color + vec3(1.0f));
	// Gamma correct
	color = pow(color, vec3(1.0f/2.2f));

	outColor = vec4(color, 1.0);

}