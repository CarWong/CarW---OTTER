#version 410
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 2) in vec3 inColor2;

layout(location = 1) out vec3 outColor;

layout(location = 2) out vec3 outColor2;

void main() {
	// vertex position in clip space
	gl_Position = vec4(inPosition, 1.0);

	outColor = inColor;

	outColor = inColor2;
}


	