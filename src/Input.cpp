#include "Input.h"

GLFWwindow* Input::sWindow = nullptr;

void Input::Init(GLFWwindow* window) {
	sWindow = window;
}

bool Input::IsKeyPressed(int32_t key)
{
	auto* window = static_cast<GLFWwindow*>(sWindow);
	auto state = glfwGetKey(window, static_cast<int32_t>(key));
	return state == GLFW_PRESS;
}

bool Input::IsKeyReleased(int32_t key)
{
	auto* window = static_cast<GLFWwindow*>(sWindow);
	auto state = glfwGetKey(window, static_cast<int32_t>(key));
	return state == GLFW_RELEASE;
}

bool Input::IsMouseButtonPressed(int32_t button)
{
	auto* window = static_cast<GLFWwindow*>(sWindow);
	auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
	return state == GLFW_PRESS;
}

bool Input::IsMouseButtonReleased(int32_t button)
{
	auto* window = static_cast<GLFWwindow*>(sWindow);
	auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
	return state == GLFW_RELEASE;
}

glm::vec2 Input::GetMousePosition()
{
	auto* window = static_cast<GLFWwindow*>(sWindow);
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	return { (float)xpos, (float)ypos };
}

float Input::GetMouseX()
{
	return GetMousePosition().x;
}

float Input::GetMouseY()
{
	return GetMousePosition().y;
}
