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

layout(location = 0) out vec3 fragColor;

void main() {

	gl_Position = vp.proj * vp.view * model.world * vec4(inPosition,1.0);
	fragColor = inColor;

}