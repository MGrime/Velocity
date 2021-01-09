#version 450

layout(binding = 0) uniform ViewProjection {
	mat4 view;
	mat4 proj;
} vp;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 TexCoords;

void main()
{
	TexCoords = inPosition;

	vec4 WVP_Pos = vp.proj * vp.view * vec4(inPosition,1.0);

	gl_Position = WVP_Pos.xyww;

}