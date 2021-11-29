#version 410
layout(location=1) in vec4 color;
layout(location=2) in vec2 texUV;

out vec4 frag_color;

uniform sampler2D myTextureSampler;

void main() {

	//LECTURE 10
	//vec4 tex = texture(myTextureSampler, texUV);
	//if (tex.r < 0.01)
	//	discard;

	//lec 11
	//frag_color = color;
	//frag_color = vec4(color, 1.0);
	//frag_color = tex;
	frag_color = texture(myTextureSampler, texUV);// * vec4(color, 1.0);
	
	
}