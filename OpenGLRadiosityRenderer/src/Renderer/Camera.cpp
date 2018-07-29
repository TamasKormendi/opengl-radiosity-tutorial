//Code adapted from https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/camera.h and corresponding tutorial

#include "stdafx.h"

#include <OpenGLGlobalHeader.h>

#include <Renderer\Camera.h>

const float Camera::YAW = -90.0f;
const float Camera::PITCH = 0.0f;
const float Camera::SPEED = 2.5f;
const float Camera::SENSITIVTY = 0.1f;
const float Camera::ZOOM = 45.0f;

Camera::Camera(glm::vec3 pos, glm::vec3 upVec, float yawAngle, float pitchAngle) : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED), mouseSensitivity(SENSITIVTY), zoom(ZOOM)
{
	position = pos;
	worldUp = upVec;
	yaw = yawAngle;
	pitch = pitchAngle;
	updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() {
	return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
	float velocity = movementSpeed * deltaTime;

	if (direction == FORWARD) {
		position += front * velocity;
	}

	if (direction == BACKWARD) {
		position -= front * velocity;
	}

	if (direction == RIGHT) {
		position += right * velocity;
	}

	if (direction == LEFT) {
		position -= right * velocity;
	}
}

void Camera::processMouse(float xOffset, float yOffset, GLboolean constrainPitch) {
	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	yaw += xOffset;
	pitch += yOffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}

	// Update Front, Right and Up Vectors using the updated Euler angles
	updateCameraVectors();
}

void Camera::updateCameraVectors() {
	glm::vec3 newFront;

	newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	newFront.y = sin(glm::radians(pitch));
	newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(newFront);

	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));
}

