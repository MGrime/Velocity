#version 450

// Im putting this here because it saves me making a vertex buffer on the vulkan side
vec3 cubeVerts[36] = vec3[](
	vec3(-1.0f,	-1.0f,	-1.0f),
	vec3(-1.0f,	-1.0f,	 1.0f),
	vec3(-1.0f,	 1.0f,	 1.0f),
	vec3( 1.0f,	 1.0f,	-1.0f),
	vec3(-1.0f,	-1.0f,	-1.0f),
	vec3(-1.0f,	 1.0f,	-1.0f),

	vec3( 1.0f,	-1.0f,	 1.0f),
	vec3(-1.0f,	-1.0f,	-1.0f),
	vec3( 1.0f,	-1.0f,	-1.0f),
	vec3( 1.0f,	 1.0f,	-1.0f),
	vec3( 1.0f,	-1.0f,	-1.0f),
	vec3(-1.0f,	-1.0f,	-1.0f),

	vec3(-1.0f,	-1.0f,	-1.0f),
	vec3(-1.0f,	 1.0f,	 1.0f),
	vec3(-1.0f,	 1.0f,	-1.0f),
	vec3( 1.0f,	-1.0f,	 1.0f),
	vec3(-1.0f,	-1.0f,	 1.0f),
	vec3(-1.0f,	-1.0f,	-1.0f),

	vec3(-1.0f,	 1.0f,	 1.0f),
	vec3(-1.0f,	-1.0f,	 1.0f),
	vec3( 1.0f,	-1.0f,	 1.0f),
	vec3( 1.0f,	 1.0f,	 1.0f),
	vec3( 1.0f,	-1.0f,	-1.0f),
	vec3( 1.0f,	 1.0f,	-1.0f),

	vec3( 1.0f,	-1.0f,	-1.0f),
	vec3( 1.0f,	 1.0f,	 1.0f),
	vec3( 1.0f,	-1.0f,	 1.0f),
	vec3( 1.0f,	 1.0f,	 1.0f),
	vec3( 1.0f,	 1.0f,	-1.0f),
	vec3(-1.0f,	 1.0f,	-1.0f),

	vec3( 1.0f,	 1.0f,	 1.0f),
	vec3(-1.0f,	 1.0f,	-1.0f),
	vec3(-1.0f,	 1.0f,	 1.0f),
	vec3( 1.0f,	 1.0f,	 1.0f),
	vec3(-1.0f,	 1.0f,	 1.0f),
	vec3( 1.0f,	-1.0f,	 1.0f)
);

layout(push_constant) uniform VP {
	mat4 projection;
	mat4 view;
} transforms;

layout(location = 0) out vec3 localPosition;

void main() {
	// Use the gl_VertexIndex work around to save using a buffer
	gl_Position = transforms.projection * transforms.view * vec4(cubeVerts[gl_VertexIndex],1.0);

	// Store local pos for UV in frag
	localPosition = cubeVerts[gl_VertexIndex];
}