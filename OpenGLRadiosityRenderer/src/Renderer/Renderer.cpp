//Parts of this file adapted from:
//https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/1.2.hello_window_clear/hello_window_clear.cpp 
//and
//http://www.opengl-tutorial.org/beginners-tutorials/tutorial-1-opening-a-window/
//Additional code in this file and in the GLSL files (Shaders folder) from https://learnopengl.com/ tutorials

#include "stdafx.h"

#include <iostream>
#include <vector>
#include <string>

//#include <OpenGLGlobalHeader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assimp\Importer.hpp>

#include <Renderer\Renderer.h>
#include <Renderer\Camera.h>
#include <Renderer\ShaderLoader.h>

#include <Renderer\ObjectModel.h>


const unsigned int SCREEN_WIDTH = 1280;
const unsigned int SCREEN_HEIGHT = 720;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void mouse_callback(GLFWwindow* window, double xPos, double yPos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

float lastX = SCREEN_WIDTH / 2.0f;
float lastY = SCREEN_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrameTime = 0.0f;

std::vector<glm::vec3> lightLocations = std::vector<glm::vec3>();

Renderer::Renderer() {

}

void Renderer::startRenderer(std::string objectFilepath) {
	if (!glfwInit()) {
		std::cout << "Failed to initialise GLFW" << std::endl;
		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Spark", NULL, NULL);

	if (window == NULL) {
		std::cout << "Failed to create window" << std::endl;
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewExperimental = true;

	if (glewInit() != GLEW_OK) {
		std::cout << "Failed to initialise GLEW" << std::endl;
		return;
	}

	glEnable(GL_DEPTH_TEST);

	ShaderLoader mainShader("../src/Shaders/MainObject.vs", "../src/Shaders/MainObject.fs");
	ShaderLoader lampShader("../src/Shaders/LampObject.vs", "../src/Shaders/LampObject.fs");

	float vertices[] = {
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
	};

	unsigned int cubeVBO;
	unsigned int cubeVAO;

	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);

	//vertexPos
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//textureCoords
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	//Lamp
	unsigned int lampVAO;
	glGenVertexArrays(1, &lampVAO);
	glBindVertexArray(lampVAO);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//Lamp position
	//glm::vec3 lampPos(1.2f, 1.0f, 2.0f);

	ObjectModel mainModel(objectFilepath);

	int frameCounter = 0;
	double fpsTimeCounter = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		++frameCounter;
		
		float currentFrameTime = glfwGetTime();

		deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		//TODO: This could be made a separate function
		if ((currentFrameTime - fpsTimeCounter) >= 1.0) {
			double actualElapsedTime = (currentFrameTime - fpsTimeCounter);

			std::cout << "mSPF: " << ((actualElapsedTime * 1000) / (double)frameCounter) << " FPS: " << ((double)frameCounter / actualElapsedTime) << std::endl;

			frameCounter = 0;
			fpsTimeCounter += actualElapsedTime;
		}

		processInput(window);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mainShader.useProgram();

		mainShader.setUniformVec3("viewPos", camera.position);
		mainShader.setUniformInt("lightAmount", lightLocations.size());

		//TODO: Ambient value is most likely going to get axed along with material.ambient

		for (int i = 0; i < lightLocations.size(); ++i) {
			mainShader.setUniformVec3("pointLights[" + std::to_string(i) + "].position", lightLocations.at(i));
			mainShader.setUniformVec3("pointLights[" + std::to_string(i) + "].ambient", 0.1f, 0.1f, 0.1f);
			mainShader.setUniformVec3("pointLights[" + std::to_string(i) + "].diffuse", 1.0f, 1.0f, 1.0f);
			mainShader.setUniformVec3("pointLights[" + std::to_string(i) + "].specular", 1.0f, 1.0f, 1.0f);

			mainShader.setUniformFloat("pointLights[" + std::to_string(i) + "].constant", 1.0f);
			mainShader.setUniformFloat("pointLights[" + std::to_string(i) + "].linear", 0.007f);
			mainShader.setUniformFloat("pointLights[" + std::to_string(i) + "].quadratic", 0.0002f);
		}


		/*mainShader.setUniformVec3("light.position", lampPos);
	
		mainShader.setUniformVec3("light.ambient", 0.1f, 0.1f, 0.1f);
		mainShader.setUniformVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
		mainShader.setUniformVec3("light.specular", 1.0f, 1.0f, 1.0f);
		mainShader.setUniformFloat("light.constant", 1.0f);
		mainShader.setUniformFloat("light.linear", 0.09f);
		mainShader.setUniformFloat("light.quadratic", 0.032f);*/

		//Material properties, likely to be handled differently later (with Assimp?)
		/*
		mainShader.setUniformVec3("material.ambient", 0.0f, 1.0f, 0.0f);
		mainShader.setUniformVec3("material.diffuse", 0.0f, 1.0f, 0.0f);
		mainShader.setUniformVec3("material.specular", 0.5f, 0.5f, 0.5f);
		mainShader.setUniformFloat("material.shininess", 32.0f);
		*/

		glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.getViewMatrix();
		mainShader.setUniformMat4("projection", projection);
		mainShader.setUniformMat4("view", view);

		glm::mat4 model;
		model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
		mainShader.setUniformMat4("model", model);

		//glBindVertexArray(cubeVAO);
		//glDrawArrays(GL_TRIANGLES, 0, 36);

		mainModel.draw(mainShader);

		lampShader.useProgram();

		lampShader.setUniformMat4("projection", projection);
		lampShader.setUniformMat4("view", view);

		glBindVertexArray(lampVAO);

		for (int i = 0; i < lightLocations.size(); ++i) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations.at(i) / 5.0f);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));
			//The uniform for the lamp's model is just "model"
			lampShader.setUniformMat4("model", lampModel);

			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

}

void Renderer::processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera.processKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera.processKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera.processKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera.processKeyboard(RIGHT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		ShaderLoader::reloadShaders();
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		glm::vec3 cameraPosition = camera.position * 5.0f;

		lightLocations.push_back(cameraPosition);
	}
}

void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
	if (firstMouse) {
		lastX = xPos;
		lastY = yPos;

		firstMouse = false;
	}

	float xOffset = xPos - lastX;
	float yOffset = lastY - yPos;

	lastX = xPos;
	lastY = yPos;

	camera.processMouse(xOffset, yOffset);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}