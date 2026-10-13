#pragma once
#include <string>
#include <vector>
#include <glm.hpp>

struct Light {
	glm::vec3 light_position;
	glm::vec3 la;
	glm::vec3 ld;
	glm::vec3 ls;
};