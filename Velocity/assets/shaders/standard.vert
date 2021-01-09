#version 450

layout(binding = 0) uniform ViewProjection {
	mat4 view;
	mat4 proj;
} vp;

layout(push_constant) uniform ObjectData {
	mat4 world;
} model;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

void main() {

	gl_Position = vp.proj * vp.view * model.world * vec4(inPosition,1.0);
	fragColor = inColor;
	fragUV = inUV;
}