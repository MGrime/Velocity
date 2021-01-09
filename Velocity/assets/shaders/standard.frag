#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform TextureData {
	layout(offset = 64) int texIndex;
} tex;


layout(binding = 1) uniform sampler2D texSampler[128];

void main() {
	
	outColor = vec4(fragColor * texture(texSampler[tex.texIndex], fragUV).rgb,1.0);
	
}