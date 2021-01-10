#version 450

layout(binding = 0) uniform ViewProjection {
	mat4 view;
	mat4 proj;
} vp;

layout(push_constant) uniform ObjectData {
	mat4 world;
} model;

layout(location = 0) in vec3 inPosition;
// Taking it in but ignoring
layout(location = 1) in vec3 inColor;		
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 TexCoords;

void main()
{
	TexCoords = inPosition;

	vec4 WVP_Pos = vp.proj * vp.view * model.world * vec4(inPosition,1.0);

	gl_Position = WVP_Pos.xyww;

}