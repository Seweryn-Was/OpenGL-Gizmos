#pragma once
#ifndef GIZMO_GUARD
#define GIZMO_GUARD


#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gizmo {
	void DecomposeTransform(const glm::mat4& modelMatrix, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
	glm::vec4 TransformVector(const glm::mat4& matrix, glm::vec4 in);

	glm::vec4 worldToScreen(const glm::vec3& worldPos, const glm::mat4& mvpMatrix, const glm::vec2& windowSize);
	void ComputeCameraRay(glm::vec4& rayOrigin, glm::vec4& rayDir);

	void setUpContext(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& model);
	float IntersectRayPlane(const glm::vec4& rOrigin, const glm::vec4& rVector, const glm::vec4& plan);

	void init();

	void manipulate(glm::mat4* view, glm::mat4* projection, glm::mat4* matrix, glm::mat4* delta); 

	void drawRotationGizmo();
}

#endif 