#version 450

layout(push_constant) uniform VP {
	mat4 projection;
	mat4 view;
} transforms;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 localPosition;

void main() {
	// Use the gl_VertexIndex work around to save using a buffer
	gl_Position = transforms.projection * transforms.view * vec4(inPosition,1.0);

	// Store local pos for UV in frag
	localPosition = inPosition;
}