#version 410
in vec3 color;
in vec2 texUV;

out vec4 frag_color;

uniform sampler2D myTextureSampler;

void main() {
	vec4 tex = texture(myTextureSampler, texUV);
	if (tex.r == 0.0)
	{
		discard;
	}

	frag_color = tex;

	//frag_color = texture(myTextureSampler, texUV);// * vec4(color, 1.0);
	
	
}