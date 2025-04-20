#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Input
{
public:
	static bool IsKeyPressed(int32_t key);
	static bool IsKeyReleased(int32_t key);

	static bool IsMouseButtonPressed(int32_t button);
	static bool IsMouseButtonReleased(int32_t button); 
	static glm::vec2 GetMousePosition();
	static float GetMouseX();
	static float GetMouseY();
	static void Init(GLFWwindow* window); 
private: 
	static GLFWwindow* sWindow; 
};
