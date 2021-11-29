#version 410
layout(location = 0) in vec3 vertex_pos;
layout(location = 1) in vec4 vertex_color;
layout(location = 2) in vec2 vertex_uv;

layout(location = 1) out vec4 outcolor;
layout(location = 2) out vec2 texUV;

uniform mat4 MVP;

void main() {
	outcolor = vertex_color;
	texUV = vertex_uv;
	//LECTURE 10
	// Flatten the object
	//vec3 vert = vertex_pos;
	//vert.x = 0.0;

	//gl_Position = MVP * vec4(vert, 1.0);
	gl_Position = MVP * vec4(vertex_pos, 1.0);
	
}
	