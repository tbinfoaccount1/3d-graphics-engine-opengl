// Controls.cpp: input en timing
#include <SDL3/SDL.h>
extern SDL_Window* gWindow; // window uit main

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
using namespace glm;

#include "controls.h"

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix() {
	return ViewMatrix;
}
glm::mat4 getProjectionMatrix() {
	return ProjectionMatrix;
}


// Beginpositie: +Z
glm::vec3 position = glm::vec3(0, 0, 5);
// Beginhoek horizontaal : naar -Z
float horizontalAngle = 3.14f;
// Beginhoek verticaal : none
float verticalAngle = 0.0f;
// Begin FOV
float initialFoV = 45.0f;

float speed = 3.0f; // 3 /s
float lookSpeed = 1.5f;


void computeMatricesFromInputs() {

	static double lastTime = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();

	// Bereken tijd verschil tussen frames
	double currentTime = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
	float deltaTime = float(currentTime - lastTime);

	int windowWidth = 1024;
	int windowHeight = 768;
	if (gWindow) {
		SDL_GetWindowSize(gWindow, &windowWidth, &windowHeight);
	}

	// Arrow key input voor rondkijken
	const bool* keyState = SDL_GetKeyboardState(NULL);
	if (keyState[SDL_SCANCODE_LEFT]) {
		horizontalAngle += lookSpeed * deltaTime;
	}
	if (keyState[SDL_SCANCODE_RIGHT]) {
		horizontalAngle -= lookSpeed * deltaTime;
	}
	if (keyState[SDL_SCANCODE_UP]) {
		verticalAngle += lookSpeed * deltaTime;
	}
	if (keyState[SDL_SCANCODE_DOWN]) {
		verticalAngle -= lookSpeed * deltaTime;
	}

	// Richting berekenen
	glm::vec3 direction(
		cos(verticalAngle) * sin(horizontalAngle),
		sin(verticalAngle),
		cos(verticalAngle) * cos(horizontalAngle)
	);

	glm::vec3 right = glm::vec3(
		sin(horizontalAngle - 3.14f / 2.0f),
		0,
		cos(horizontalAngle - 3.14f / 2.0f)
	);

	glm::vec3 up = glm::cross(right, direction);

	// WASD voor beweging
	if (keyState[SDL_SCANCODE_W]) {
		position += direction * deltaTime * speed;
	}
	if (keyState[SDL_SCANCODE_S]) {
		position -= direction * deltaTime * speed;
	}
	if (keyState[SDL_SCANCODE_D]) {
		position += right * deltaTime * speed;
	}
	if (keyState[SDL_SCANCODE_A]) {
		position -= right * deltaTime * speed;
	}

	// FOV functie gaan we later doen
	float FoV = initialFoV;// - 5 * someSDLscrollwheelFunction();

	// Projection matrix : FOV, 4:3 ratio, range : 0.1 unit <-> 100 units
	ProjectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 100.0f);
	// Camera matrix
	ViewMatrix = glm::lookAt(
		position,           // Camera is hier
		position + direction, // kijkt hiernaartoe
		up                  // Head is omhoog
	);

	lastTime = currentTime;
}