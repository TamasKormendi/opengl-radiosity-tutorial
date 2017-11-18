#ifndef CAMERA_H
#define CAMERA_H

enum CameraMovement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};



class Camera {
public:
	static const float YAW;
	static const float PITCH;
	static const float SPEED;
	static const float SENSITIVTY;
	static const float ZOOM;

	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

	float yaw;
	float pitch;

	float movementSpeed;
	float mouseSensitivity;
	float zoom;

	Camera(
		glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 upVec = glm::vec3(0.0f, 1.0f, 0.0f),
		float yawAngle = YAW,
		float pitchAngle = PITCH);

	glm::mat4 getViewMatrix();

	void processKeyboard(CameraMovement direction, float deltaTime);

	void processMouse(float xOffset, float yOffset, GLboolean constrainPitch = true);

private:

	void updateCameraVectors();
	
};

#endif
