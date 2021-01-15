#version 450

layout(location = 0) in vec3 TexCoords;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform samplerCube skybox;

void main()
{
	vec3 envColor = texture(skybox,TexCoords).rgb;

	envColor = envColor / (envColor + vec3(1.0f));
	envColor = pow(envColor,vec3(1.0/2.2));

	outColor = vec4(envColor,1.0);

}