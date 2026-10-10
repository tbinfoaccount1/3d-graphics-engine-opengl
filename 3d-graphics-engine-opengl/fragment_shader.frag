#version 330 core

// UV van vertex shader
in vec2 UV;

// Output data
out vec3 color;

// Constante texture sampler
uniform sampler2D myTextureSampler;

void main(){

	// Output kleur is kleur van texture op UV.
	color = texture( myTextureSampler, UV ).rgb;
}