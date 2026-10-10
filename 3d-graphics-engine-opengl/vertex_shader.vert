#version 330 core

// Input vertex data
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexUV;

// Output data
out vec2 UV;

// Constante MVP matrix
uniform mat4 MVP;

void main(){

	// Output positie van vertex met MVP matrix
	gl_Position =  MVP * vec4(vertexPosition,1);
	
	// UV van matrix naar fragment shader sturen
	UV = vertexUV;
}