#include "Gizmo.h"  
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "openglUtil.h"
#include "shaderprogram.h"
#include "Input.h"

#define PI 3.14159f

double mouseX = 0.0, mouseY = 0.0;
float radius = 0.2f;
uint32_t numSegments = 100;

ShaderProgram gDefaultShader;
// Define the vertices of the half ring

std::vector<GLfloat> vertices[3];
std::vector<GLuint> indices[3];
GLuint VAO[3], VBO[3], EBO[3];
GLuint circVAO, circVBO, circEBO;

struct Context {

	Context() :usingGizmo(false) {};

	glm::mat4 viewMat;
	glm::mat4 projectionMat;
	glm::mat4 model;
	glm::vec3 modelScaleOrigin;
	glm::mat4 mModelSource;

	glm::vec4 cameraEye;

	glm::mat4 mvp;
	glm::vec4 gizmoCenter;

	glm::vec4 mRayOrigin;
	glm::vec4 mRayVector;

	float rotationAngleOrigin;
	float rotationAngle;
	glm::vec4 translationPlan;

	glm::vec4 mRotationVectorSource;

	uint32_t wWidth, wHeight;

	uint32_t type;
	uint32_t mainType;
	bool usingGizmo;
};

struct Context gContext;

namespace gizmo {


	void init() {
		gDefaultShader = ShaderProgram("shaders/v_default.glsl", "shaders/f_default.glsl");

		for (unsigned int axis = 0; axis < 3; axis++)
		{

			for (int i = 0; i <= static_cast<int>(numSegments); i++) {
				float angle = (float)i / (float)numSegments * PI;
				float x = radius * cos(angle);
				float y = radius * sin(angle);
				(vertices[axis]).push_back(x);
				(vertices[axis]).push_back(y);
				(vertices[axis]).push_back(0.0f);
			}

			for (int i = 0; i < static_cast<int>(numSegments); i++) {
				(indices[axis]).push_back(i);
				(indices[axis]).push_back(i + 1);
			}

			glGenVertexArrays(1, &VAO[axis]);
			glGenBuffers(1, &VBO[axis]);
			glGenBuffers(1, &EBO[axis]);

			glBindVertexArray(VAO[axis]);

			// Load vertex data into VBO
			glBindBuffer(GL_ARRAY_BUFFER, VBO[axis]);
			glBufferData(GL_ARRAY_BUFFER, vertices[axis].size() * sizeof(GLfloat), vertices[axis].data(), GL_DYNAMIC_DRAW);

			// Load index data into EBO
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[axis]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices[axis].size() * sizeof(GLuint), indices[axis].data(), GL_DYNAMIC_DRAW);

			// Set up vertex attributes (assuming 2D positions)
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(0);

			//glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		std::vector<glm::vec3> circleVert(numSegments);

		for (int i = 0; i < numSegments; ++i) {
			float theta = 2 * PI * float(i) / float(numSegments);
			glm::vec3 point = glm::vec3(cos(theta), sin(theta), 0.0f) * (radius + 0.03f);
			circleVert[i] = point; 
		}

		glGenVertexArrays(1, &circVAO);
		glGenBuffers(1, &circVBO);
		glGenBuffers(1, &circEBO);

		glBindVertexArray(circVAO);

		glBindBuffer(GL_ARRAY_BUFFER, circVBO);
		glBufferData(GL_ARRAY_BUFFER, circleVert.size() * sizeof(glm::vec3), circleVert.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
	}

	void DecomposeTransform(const glm::mat4& modelMatrix, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale) {
		// Extract the translation (last column)
		translation = glm::vec3(modelMatrix[3]);

		// Extract the scale (length of each axis in the upper-left 3x3 part of the matrix)
		scale.x = glm::length(glm::vec3(modelMatrix[0]));
		scale.y = glm::length(glm::vec3(modelMatrix[1]));
		scale.z = glm::length(glm::vec3(modelMatrix[2]));

		// Remove scaling from the upper-left 3x3 matrix to get rotation matrix
		glm::mat3 rotationMatrix = glm::mat3(modelMatrix);
		rotationMatrix[0] /= scale.x;
		rotationMatrix[1] /= scale.y;
		rotationMatrix[2] /= scale.z;

		// Convert rotation matrix to quaternion
		glm::quat rotationQuat = glm::quat_cast(rotationMatrix);

		// Convert quaternion to Euler angles in degrees
		rotation = glm::degrees(glm::eulerAngles(rotationQuat));
	}

	// apply to vector <in> only scale and roattion from matrix <matrix>
	glm::vec4 TransformVector(const glm::mat4& matrix, glm::vec4 in) {
		glm::vec4 out(0.0f);
		float x = in.x, y = in.y, z = in.z, w = in.w;

		out.x = x * matrix[0][0] + y * matrix[1][0] + z * matrix[2][0];
		out.y = x * matrix[0][1] + y * matrix[1][1] + z * matrix[2][1];
		out.z = x * matrix[0][2] + y * matrix[1][2] + z * matrix[2][2];
		out.w = x * matrix[0][3] + y * matrix[1][3] + z * matrix[2][3];

		return out;
	}



	glm::vec4 worldToScreen(const glm::vec3& worldPos, const glm::mat4& mvpMatrix, const glm::vec2& windowSize)
	{
		// Transform the world position to homogeneous clip space
		glm::vec4 clipSpacePos = mvpMatrix * glm::vec4(worldPos, 1.0f);

		// Perspective divide to get normalized device coordinates (NDC)
		glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;

		// Convert from NDC [-1,1] to screen space [0, windowSize]
		glm::vec2 screenPos(0.0);
		screenPos.x = (ndcSpacePos.x + 1.0f) * 0.5f * windowSize.x;
		screenPos.y = (1.0f - ndcSpacePos.y) * 0.5f * windowSize.y; // Y is flipped for screen space

		return glm::vec4(screenPos, 0.0f, 0.0f); // Return as 2D screen space position (x, y)
	}


	void ComputeCameraRay(glm::vec4& rayOrigin, glm::vec4& rayDir) {

		// Compute inverse view-projection matrix
		glm::mat4 mViewProjInverse = glm::inverse(gContext.projectionMat * gContext.viewMat);

		// Normalize mouse coordinates to NDC space (-1 to 1)
		float mox = (Input::GetMouseX() / gContext.wWidth) * 2.0f - 1.0f;
		float moy = (1.0f - (Input::GetMouseY() / gContext.wHeight)) * 2.0f - 1.0f;
		// Define near and far depth values
		float zNear = 0.0f;
		float zFar = (1.0f - FLT_EPSILON);

		// Compute ray origin in world space
		rayOrigin = mViewProjInverse * glm::vec4(mox, moy, zNear, 1.0f);
		rayOrigin /= rayOrigin.w; // Perspective divide

		// Compute ray end point in world space
		glm::vec4 rayEnd = mViewProjInverse * glm::vec4(mox, moy, zFar, 1.0f);
		rayEnd /= rayEnd.w; // Perspective divide

		// Compute ray direction (normalized)
		rayDir = glm::normalize(rayEnd - rayOrigin);
	}

	glm::mat4 RemoveScale(const glm::mat4& matrix) {
		glm::vec3 translation = glm::vec3(matrix[3]);

		// Extract and normalize the rotation axes (columns 0, 1, 2)
		glm::vec3 xAxis = glm::normalize(glm::vec3(matrix[0]));
		glm::vec3 yAxis = glm::normalize(glm::vec3(matrix[1]));
		glm::vec3 zAxis = glm::normalize(glm::vec3(matrix[2]));

		// Create a new matrix without scale
		glm::mat4 result(1.0f);
		result[0] = glm::vec4(xAxis, 0.0f);
		result[1] = glm::vec4(yAxis, 0.0f);
		result[2] = glm::vec4(zAxis, 0.0f);
		result[3] = glm::vec4(translation, 1.0f);

		return result;
	}

	glm::vec3 GetScaleFromMatrix(const glm::mat4& matrix) {
		glm::vec3 scale(1.0);
		scale.x = glm::length(glm::vec3(matrix[0])); // column 0 = X axis
		scale.y = glm::length(glm::vec3(matrix[1])); // column 1 = Y axis
		scale.z = glm::length(glm::vec3(matrix[2])); // column 2 = Z axis
		return scale;
	}

	glm::mat4 drawModel = glm::mat4(1.0f); 

	void setUpContext(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& model) {
		drawModel = model;
		gContext.viewMat = view;
		gContext.projectionMat = projection;
		gContext.modelScaleOrigin = GetScaleFromMatrix(model);
		gContext.model =  RemoveScale(model);
 
		gContext.mvp = projection * view * gContext.model;

		glm::mat4 invView = glm::inverse(view);

		gContext.wWidth = 1200;
		gContext.wHeight = 800;

		gContext.cameraEye = invView[3];

		gContext.gizmoCenter = gContext.mvp * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		gContext.gizmoCenter *= 0.5f / gContext.gizmoCenter.w;
		gContext.gizmoCenter.x += 0.5f;
		gContext.gizmoCenter.y += 0.5f;
		gContext.gizmoCenter.y = 1.0f - gContext.gizmoCenter.y;
		//gContext.gizmoCenter += glm::vec4(0.5f * 800, 0.5f * 600, 0.0f, 0.0f);

		gContext.gizmoCenter.x *= gContext.wWidth;
		gContext.gizmoCenter.y *= gContext.wHeight;

		ComputeCameraRay(gContext.mRayOrigin, gContext.mRayVector);
	}

	float IntersectRayPlane(const glm::vec4& rOrigin, const glm::vec4& rVector, const glm::vec4& plan)
	{
		const float numer = glm::dot(glm::vec3(plan), glm::vec3(rOrigin)) - plan.w; // plan.Dot3(rOrigin) - plan.w;
		const float denom = glm::dot(glm::vec3(plan), glm::vec3(rVector)); //plan.Dot3(rVector);

		if (fabsf(denom) < FLT_EPSILON)  // normal is orthogonal to vector, cant intersect
		{
			return -1.0f;
		}

		return -(numer / denom);
	}

	void manipulate(glm::mat4* view, glm::mat4* projection, glm::mat4* matrix, glm::mat4* delta) {
		setUpContext(*view, *projection, *matrix);

		glm::vec2 deltaScreen = { Input::GetMouseX() - gContext.gizmoCenter.x, Input::GetMouseY() - gContext.gizmoCenter.y };

		float dist = sqrtf(deltaScreen.x * deltaScreen.x + deltaScreen.y * deltaScreen.y);

		const glm::vec4 planNormals[3] = { gContext.model[2], gContext.model[1], gContext.model[0] };

		glm::vec4 modelViewPos = gContext.viewMat * gContext.model[3];
		gContext.type = 0;
		for (int i = 0; i < 3; i++)
		{
			// Compute the plane equation in world space
			glm::vec4 normal = glm::normalize(planNormals[i]);
			glm::vec4 pickupPlane = glm::vec4(normal.x, normal.y, normal.z, glm::dot(normal, gContext.model[3]));

			// Intersect ray with plane
			float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, pickupPlane);

			glm::vec4 intersectWorldPos = gContext.mRayOrigin + gContext.mRayVector * len;
			glm::vec4 intersectViewPos = gContext.viewMat * intersectWorldPos;

			if (fabsf(modelViewPos.z) - fabsf(intersectViewPos.z) < -FLT_EPSILON)
			{
				continue;
			}

			// Compute local position
			glm::vec4 localPos = intersectWorldPos - gContext.model[3];
			glm::vec4 idealPosOnCircle = glm::normalize(localPos) * radius;  // gContext.rotationDisplayFactor;
			idealPosOnCircle = TransformVector(glm::inverse(gContext.model), idealPosOnCircle);
			// Convert to screen space
			glm::vec4 trans;
			trans = gContext.mvp * glm::vec4(glm::vec3(idealPosOnCircle), 1.0f);
			glm::vec3 ndc = glm::vec3(trans.x, trans.y, trans.z) / trans.w;
			ndc.x *= 0.5f;
			ndc.y *= 0.5f;
			ndc.x += 0.5f;
			ndc.y += 0.5f;

			ndc.x *= gContext.wWidth;
			ndc.y *= gContext.wHeight;

			const glm::vec2 idealPosOnCircleScreen = glm::vec2(ndc.x, ndc.y);

			glm::vec2 mousePos = Input::GetMousePosition();
			mousePos.y = gContext.wHeight - mousePos.y;

			glm::vec2 distanceOnScreen = idealPosOnCircleScreen - mousePos;
			float distance = glm::length(distanceOnScreen);

			//To Fix: hover over ring detection is not working corectly; incorect distance detection on screem

			if (distance < 15.0f) // Pixel threshold for selection
			{
				gContext.type = 1 + i;
			}
		}


		if (!gContext.usingGizmo) {
			gContext.mainType = gContext.type;

			if (gContext.mainType != 0 && Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
				gContext.usingGizmo = true;
				glm::vec4 rotatePlanNormal[3] = { gContext.model[2], gContext.model[1], gContext.model[0] };
				glm::vec4 normal = glm::normalize(rotatePlanNormal[gContext.mainType - 1]);
				gContext.translationPlan = glm::vec4(normal.x, normal.y, normal.z, glm::dot(normal, gContext.model[3]));

				const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.translationPlan);
				glm::vec4 localPos = gContext.mRayOrigin + gContext.mRayVector * len - gContext.model[3];
				gContext.mRotationVectorSource = glm::normalize(localPos);

				glm::vec4 perpendicularVector = glm::vec4(glm::cross(glm::vec3(gContext.mRotationVectorSource), glm::vec3(gContext.translationPlan)), 0.0f);
				perpendicularVector = glm::normalize(perpendicularVector);
				float acosAngle = glm::clamp(glm::dot(glm::normalize(localPos), gContext.mRotationVectorSource), -1.0f, 1.0f);
				float angle = acosf(acosAngle);
				angle *= (glm::dot(localPos, perpendicularVector) < 0.f) ? 1.f : -1.f;
				gContext.rotationAngleOrigin = angle;
				gContext.mModelSource = gContext.model;

			}
		}

		if (gContext.usingGizmo) {

			const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.translationPlan);
			glm::vec4 localPos = gContext.mRayOrigin + gContext.mRayVector * len - gContext.mModelSource[3];

			glm::vec4 perpendicularVector = glm::vec4(glm::cross(glm::vec3(gContext.mRotationVectorSource), glm::vec3(gContext.translationPlan)), 0.0f);
			perpendicularVector = glm::normalize(perpendicularVector);
			float acosAngle = glm::clamp(glm::dot(glm::normalize(localPos), gContext.mRotationVectorSource), -1.0f, 1.0f);
			float angle = acosf(acosAngle);
			angle *= (glm::dot(localPos, perpendicularVector) < 0.f) ? 1.f : -1.f;
			gContext.rotationAngle = angle;

			glm::vec4 rotationAxisLocalSpace = TransformVector(glm::inverse(gContext.mModelSource), glm::vec4(gContext.translationPlan.x, gContext.translationPlan.y, gContext.translationPlan.z, 0.0f));
			rotationAxisLocalSpace = glm::normalize(rotationAxisLocalSpace);

			float deltaAngle = gContext.rotationAngle - gContext.rotationAngleOrigin;
			
			glm::mat4 deltamat = glm::rotate(gContext.model, deltaAngle, glm::vec3(rotationAxisLocalSpace));

			glm::mat4 result = deltamat; 
			result[3] = gContext.mModelSource[3];
			result = glm::scale(result, gContext.modelScaleOrigin);

			//gContext.model = deltamat;

			gContext.rotationAngleOrigin = gContext.rotationAngle;

			if (Input::IsMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT)) {
				gContext.usingGizmo = false;
				gContext.type = 0;
			}

			*matrix = result; //gContext.model;
			*delta = glm::rotate(gContext.model, deltaAngle, glm::vec3(rotationAxisLocalSpace));//deltamat; 
		}


	}

	void drawRotationGizmo() {
		glm::vec4 cameraToModelNormalized = glm::normalize(gContext.model[3] - gContext.cameraEye);
		cameraToModelNormalized = TransformVector(glm::inverse(gContext.model), cameraToModelNormalized);

		for (unsigned int axis = 0; axis < 3; axis++) {
			float angleStart = atan2f(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + PI * 0.5f;
			vertices[axis].clear();
			for (int i = 0; i <= static_cast<int>(numSegments); i++) {
				float angle = angleStart + (float)i / (float)numSegments * PI;
				float x = radius * cos(angle);
				float y = radius * sin(angle);

				glm::vec4 pos = glm::vec4(x, y, 0.0f, 1.0f);

				glm::vec4 axisPos = glm::vec4(pos[axis], pos[(axis + 1) % 3], pos[(axis + 2) % 3], 1.0f) ; //* 1.5f

				vertices[axis].push_back(axisPos.x);
				vertices[axis].push_back(axisPos.y);
				vertices[axis].push_back(axisPos.z);
			}
			glm::vec3 axisColor = (axis == 0) ? glm::vec3(1.0f, 0.0f, 0.0f) : (axis == 1) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);

			if (axis == gContext.mainType - 1) {
				axisColor = glm::vec3(1.0f, 0.5f, 0.0f);
			}

			glDisable(GL_DEPTH_TEST);
			gDefaultShader.use();

			glBindVertexArray(VAO[axis]);
			glBindBuffer(GL_ARRAY_BUFFER, VBO[axis]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices[axis].size(), vertices[axis].data(), GL_DYNAMIC_DRAW);

			glUniformMatrix4fv(gDefaultShader.u("V"), 1, GL_FALSE, glm::value_ptr(gContext.viewMat));
			glUniformMatrix4fv(gDefaultShader.u("P"), 1, GL_FALSE, glm::value_ptr(gContext.projectionMat));

			glUniform3f(gDefaultShader.u("color"), axisColor.r, axisColor.g, axisColor.b);
			glUniformMatrix4fv(gDefaultShader.u("M"), 1, GL_FALSE, glm::value_ptr(glm::mat4(gContext.model)));
			glLineWidth(3.0f);
			glDrawElements(GL_LINES, static_cast<GLsizei>(indices[axis].size()), GL_UNSIGNED_INT, 0);
			glEnable(GL_DEPTH_TEST);
		}
		
		glDisable(GL_DEPTH_TEST);
		gDefaultShader.use();

		glBindVertexArray(circVAO);

		glUniformMatrix4fv(gDefaultShader.u("V"), 1, GL_FALSE, glm::value_ptr(gContext.viewMat));
		glUniformMatrix4fv(gDefaultShader.u("P"), 1, GL_FALSE, glm::value_ptr(gContext.projectionMat));

		glm::vec3 objectPos = glm::vec3(gContext.model[3]);
		glm::vec3 cameraPos = glm::vec3(glm::inverse(gContext.viewMat)[3]);

		glm::vec3 viewDir = glm::normalize(cameraPos - objectPos);

		glm::vec3 defaultNormal = glm::vec3(0, 0, 1);
		glm::quat rot = glm::rotation(defaultNormal, viewDir);
		glm::mat4 rotationMatrix = glm::toMat4(rot);
		glm::mat4 billboardModel = glm::translate(glm::mat4(1.0f), objectPos) * rotationMatrix;

		glUniformMatrix4fv(gDefaultShader.u("M"), 1, GL_FALSE, glm::value_ptr(billboardModel));

		glUniform3f(gDefaultShader.u("color"), 0.5f, 0.5f, 0.5f);
		glLineWidth(3.0f);
		glDrawArrays(GL_LINE_LOOP, 0, numSegments);

		glEnable(GL_DEPTH_TEST);
		
	}
}
