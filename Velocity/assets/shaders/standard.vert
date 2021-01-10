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

	gl_Position = vp.proj * vp.view * model.world * vec4(inPosition,1.0);

	fragPosition = inPosition;
	fragNormal = inNormal;
	fragUV = inUV;
}