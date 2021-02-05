/*
* Basic camera class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>

class Camera
{
private:
	float fov = 90.0f;
	float zNear = 1.0f, zFar = 1000.0f;

	void UpdateViewMatrix()
	{
		glm::quat qPitch = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::quat qYaw = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::quat qRoll = glm::angleAxis(roll, glm::vec3(0.0f, 0.0f, 1.0f));

		glm::quat orientation = qPitch * qYaw;
		orientation = glm::normalize(orientation);
		const glm::mat4 rotM = glm::mat4_cast(orientation);

		glm::vec3 translation = position;
		if (flipY) {
			translation.y *= -1.0f;
		}
		const glm::mat4 transM = glm::translate(glm::mat4(1.0f), translation);

		matrices.view = type == CameraType::FirstPerson ? rotM * transM : transM * rotM;

		viewPos = glm::vec4(position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);
		camFront = glm::normalize(static_cast<glm::vec3>(glm::inverse(matrices.view)[2]) * glm::vec3(1.0f, 1.0f, 1.0f));

		updated = true;
	}
public:
	enum class CameraType { LookAt, FirstPerson };
	CameraType type = CameraType::FirstPerson;

	glm::vec3 position = glm::vec3();
	glm::vec4 viewPos = glm::vec4();
	// glm::vec3 rotation = glm::vec3();
	glm::vec2 cursorChange = glm::vec2();
	glm::vec3 camFront = { 0.0f, 0.0f, 1.0f };

	float pitch = 0.0f;
	float yaw = 0.0f;
	float roll = 0.0f;

	float rotationSpeed = 1.0f;
	float movementSpeed = 10.0f;

	bool updated = false;
	bool flipY = false;

	struct
	{
		glm::mat4 perspective;
		glm::mat4 view;
	} matrices;

	struct
	{
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	} keys;

	bool Moving()
	{
		return keys.left || keys.right || keys.up || keys.down;
	}

	float GetNearClip() {
		return zNear;
	}

	float GetFarClip() {
		return zFar;
	}

	void SetPerspective(float fov, float aspect, float zNear, float zFar)
	{
		this->fov = fov;
		this->zNear = zNear;
		this->zFar = zFar;
		matrices.perspective = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
		if (flipY) {
			matrices.perspective[1][1] *= -1.0f;
		}
	}

	void UpdateAspectRatio(float aspect)
	{
		matrices.perspective = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
		if (flipY) {
			matrices.perspective[1][1] *= -1.0f;
		}
	}

	void SetPosition(glm::vec3 position)
	{
		this->position = position;
		UpdateViewMatrix();
	}

	// void SetRotation(glm::vec3 rotation)
	// {
	// 	this->rotation = rotation;
	// 	UpdateViewMatrix();
	// }
	//
	// void Rotate(glm::vec3 delta)
	// {
	// 	this->rotation += delta;
	// 	UpdateViewMatrix();
	// }

	void SetTranslation(glm::vec3 translation)
	{
		this->position = translation;
		UpdateViewMatrix();
	}

	void Translate(glm::vec3 delta)
	{
		this->position += delta;
		UpdateViewMatrix();
	}

	void SetRotationSpeed(float rotationSpeed)
	{
		this->rotationSpeed = rotationSpeed;
	}

	void SetMovementSpeed(float movementSpeed)
	{
		this->movementSpeed = movementSpeed;
	}

	void ProcessMouseMovement(glm::vec2 cursorPos)
	{
		static glm::vec2 prevCursorPos = cursorPos;

		cursorChange = cursorPos - prevCursorPos;

		prevCursorPos = cursorPos;
	}
	

	void Update(float deltaTime)
	{
		updated = false;
		if (type == CameraType::FirstPerson)
		{
			// glm::vec3 camFront;
			// camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
			// camFront.y = sin(glm::radians(rotation.x));
			// camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
			// camFront = glm::normalize(camFront);
			float moveSpeed = deltaTime * movementSpeed;
			float rotSpeed = deltaTime * rotationSpeed;

			// pitch = glm::lerp<float>(pitch, pitch + cursorChange.y * rotSpeed, 0.95f);
			// yaw = glm::lerp<float>(yaw, yaw + cursorChange.x * rotSpeed, 0.95f);
			pitch += cursorChange.y * rotSpeed;
			yaw += cursorChange.x * rotSpeed;
			cursorChange = { 0.0f, 0.0f };

			if (keys.up)
				position += camFront * moveSpeed;
			if (keys.down)
				position -= camFront * moveSpeed;
			if (keys.left)
				position -= glm::normalize(glm::cross(static_cast<glm::vec3>(camFront), glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
			if (keys.right)
				position += glm::normalize(glm::cross(static_cast<glm::vec3>(camFront), glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;

			UpdateViewMatrix();
		}
	}
};
