#version 410
layout(location = 0) in vec3 vertex_pos;
layout(location = 1) in vec4 vertex_color;
layout(location = 2) in vec2 vertex_uv;

out vec4 color;
out vec2 texUV;

uniform mat4 MVP;

uniform sampler2D myTextureSampler;

uniform float delta;

void main() {
	
	color = vertex_color;
	texUV = vertex_uv;

	vec3 vert = vertex_pos;
	//vert.y = texture(myTextureSampler, vertex_uv).r;

	//sin animation
	if( color.b > 0.5)
	{
		vert.y = (sin(vert.x * 5.0 + (delta/2)) * 0.1) + (sin(vert.z * 5.0 + (delta/2)) * 0.1);
	}
	//vert.x = sin(vert.y * 1.0 + delta) * 0.2;

	gl_Position = MVP * vec4(vert, 1.0);
	//gl_Position = MVP * vec4(vertex_pos, 1.0);
}
	