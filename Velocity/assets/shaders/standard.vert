#version 450

layout(binding = 0) uniform ViewProjection {
	mat4 view;
	mat4 proj;
} vp;

layout(push_constant) uniform ObjectData {
	mat4 world;
} model;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;

void main() {

	// For multiplications
	vec4 modelPosition = vec4(inPosition, 1.0f);

	// Transform position into world space
	vec4 worldPosition = model.world * modelPosition;

	// Transform in view space
	vec4 viewPosition = vp.view * worldPosition;
	
	// Finally into projection space and output
	gl_Position = vp.proj * viewPosition;

	// For multiplications
	vec4 modelNormal = vec4(inNormal, 0.0f);

	fragNormal = (model.world * modelNormal).xyz;

	fragPosition = worldPosition.xyz;

	fragUV = inUV;
	
}