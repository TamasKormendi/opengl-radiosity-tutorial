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

#include <Renderer\RadiosityConfig.h>


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
bool addLampMesh = false;
bool addAmbient = false;

bool doRadiosityIteration = false;

//TODO: Change this to a sensible ENUM
unsigned int preprocessDone = 0;

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

	//ShaderLoader mainShader("../src/Shaders/MainObject.vs", "../src/Shaders/MainObject.fs");
	//ShaderLoader lampShader("../src/Shaders/LampObject.vs", "../src/Shaders/LampObject.fs");
	ShaderLoader preprocessShader("../src/Shaders/preprocess.vs", "../src/Shaders/preprocess.fs");
	ShaderLoader visibilityTextureShader("../src/Shaders/HemisphereVisibilityTexture.vs", "../src/Shaders/HemisphereVisibilityTexture.fs");
	ShaderLoader lightmapUpdateShader("../src/Shaders/LightmapUpdate.vs", "../src/Shaders/LightmapUpdate.fs");
	ShaderLoader finalRenderShader("../src/Shaders/FinalRender.vs", "../src/Shaders/FinalRender.fs");

	ObjectModel mainModel(objectFilepath, false);
	ObjectModel lampModel("../assets/lamp.obj", true);

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

			//std::cout << "mSPF: " << ((actualElapsedTime * 1000) / (double)frameCounter) << " FPS: " << ((double)frameCounter / actualElapsedTime) << std::endl;

			frameCounter = 0;
			fpsTimeCounter += actualElapsedTime;
		}

		processInput(window);

		if (addLampMesh) {
			mainModel.addMesh(lampModel.meshes[0]);

			addLampMesh = !addLampMesh;
		}

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		finalRenderShader.useProgram();

		//mainShader.setUniformVec3("viewPos", camera.position);
		//mainShader.setUniformInt("lightAmount", lightLocations.size());

		//TODO: Ambient value is most likely going to get axed along with material.ambient
		
		/*
		for (int i = 0; i < lightLocations.size(); ++i) {
			mainShader.setUniformVec3("pointLights[" + std::to_string(i) + "].position", lightLocations.at(i));
			mainShader.setUniformVec3("pointLights[" + std::to_string(i) + "].ambient", 0.1f, 0.1f, 0.1f);
			mainShader.setUniformVec3("pointLights[" + std::to_string(i) + "].diffuse", 1.0f, 1.0f, 1.0f);
			mainShader.setUniformVec3("pointLights[" + std::to_string(i) + "].specular", 1.0f, 1.0f, 1.0f);

			mainShader.setUniformFloat("pointLights[" + std::to_string(i) + "].constant", 1.0f);
			mainShader.setUniformFloat("pointLights[" + std::to_string(i) + "].linear", 0.007f);
			mainShader.setUniformFloat("pointLights[" + std::to_string(i) + "].quadratic", 0.0002f);
		}
		*/

		glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
		glm::mat4 ortho = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
		glm::mat4 view = camera.getViewMatrix();
		finalRenderShader.setUniformMat4("projection", projection);
		//mainShader.setUniformMat4("ortho", ortho);
		finalRenderShader.setUniformMat4("view", view);

		glm::mat4 model;
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
		finalRenderShader.setUniformMat4("model", model);

		finalRenderShader.setUniformBool("addAmbient", addAmbient);

		glEnable(GL_CULL_FACE);

		if (preprocessDone == 1) {

			preprocess(mainModel, preprocessShader, model);

			/*std::cout << mainModel.meshes[7].uvData[1533] << std::endl;
			std::cout << mainModel.meshes[7].uvData[1534] << std::endl;
			std::cout << mainModel.meshes[7].uvData[1535] << std::endl;*/

			//This is needed so preprocess can only be done once
			++preprocessDone;
		}

		glm::vec3 shooterRadiance;
		glm::vec3 shooterWorldspacePos;
		glm::vec3 shooterWorldspaceNormal;
		glm::vec2 shooterUV;
		int shooterMeshIndex;

		if (doRadiosityIteration) {

			selectShooter(mainModel, shooterRadiance, shooterWorldspacePos, shooterWorldspaceNormal, shooterUV, shooterMeshIndex);

			/*
			std::cout << shooterUV.x << std::endl;
			std::cout << shooterUV.y << std::endl;
			//std::cout << shooterUV.z << std::endl;
			*/

			glm::mat4 shooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + shooterWorldspaceNormal, glm::vec3(0, 1, 0));

			unsigned int visibilityTextureSize = 1024;

			unsigned int visibilityTexture = createVisibilityTexture(mainModel, visibilityTextureShader, model, shooterView, visibilityTextureSize);

			lightmapUpdateShader.useProgram();

			lightmapUpdateShader.setUniformVec3("shooterRadiance", shooterRadiance);
			lightmapUpdateShader.setUniformVec3("shooterWorldspacePos", shooterWorldspacePos);
			lightmapUpdateShader.setUniformVec3("shooterWorldspaceNormal", shooterWorldspaceNormal);
			lightmapUpdateShader.setUniformVec2("shooterUV", shooterUV);

			updateLightmaps(mainModel, lightmapUpdateShader, model, shooterView, visibilityTexture);

			doRadiosityIteration = false;
		}

		

		int lampCounter = 0;

		for (unsigned int i = 0; i < mainModel.meshes.size(); ++i) {
			if (mainModel.meshes[i].isLamp) {
				finalRenderShader.useProgram();

				finalRenderShader.setUniformMat4("projection", projection);
				finalRenderShader.setUniformMat4("view", view);

				glm::mat4 lampModel = glm::mat4();

				lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
				lampModel = glm::scale(lampModel, glm::vec3(0.05f));
				//The uniform for the lamp's model is just "model"
				finalRenderShader.setUniformMat4("model", lampModel);

				
				unsigned int irradianceID;
				glGenTextures(1, &irradianceID);

				glBindTexture(GL_TEXTURE_2D, irradianceID);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, &mainModel.meshes[i].irradianceData[0]);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

				glActiveTexture(GL_TEXTURE0);
				finalRenderShader.setUniformInt("irradianceTexture", 0);
				glBindTexture(GL_TEXTURE_2D, irradianceID);
				

				mainModel.meshes[i].draw(finalRenderShader);

				glDeleteTextures(1, &irradianceID);

				++lampCounter;
			}
			else {
				finalRenderShader.useProgram();

				
				unsigned int irradianceID;
				glGenTextures(1, &irradianceID);

				glBindTexture(GL_TEXTURE_2D, irradianceID);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, &mainModel.meshes[i].irradianceData[0]);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

				glActiveTexture(GL_TEXTURE0);
				finalRenderShader.setUniformInt("irradianceTexture", 0);
				glBindTexture(GL_TEXTURE_2D, irradianceID);
				

				mainModel.meshes[i].draw(finalRenderShader);

				//glDeleteTextures(1, &irradianceID);
			}
		}
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

}

//I'm fairly sure most of the FBOs and maybe most of the textures can be created as a preprocess step, which should result in significant speedup


//This function will be called automatically before the scene starts rendering eventually so preprocessing can be done before the rendering
//Current issues:
//Light placement (have to know the location of the light when doing processing, can be split into 2 parts - preprocess most of the scene and process lights at creation)
//It is somewhat untested
void Renderer::preprocess(ObjectModel& model, ShaderLoader& shader, glm::mat4& mainObjectModelMatrix) {
	unsigned int preprocessFramebuffer;

	glGenFramebuffers(1, &preprocessFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, preprocessFramebuffer);

	unsigned int worldspacePosData;
	unsigned int worldspaceNormalData;

	unsigned int idData;
	unsigned int uvData;

	//Create texture to hold worldspace-position data
	glGenTextures(1, &worldspacePosData);
	glBindTexture(GL_TEXTURE_2D, worldspacePosData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, worldspacePosData, 0);

	//Create texture to hold worldspace-normal data
	glGenTextures(1, &worldspaceNormalData);
	glBindTexture(GL_TEXTURE_2D, worldspaceNormalData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, worldspaceNormalData, 0);

	//Create texture to hold triangle IDs in texturespace
	glGenTextures(1, &idData);
	glBindTexture(GL_TEXTURE_2D, idData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, idData, 0);

	//Create texture to hold UV-coordinate data in texturespace
	glGenTextures(1, &uvData);
	glBindTexture(GL_TEXTURE_2D, uvData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, uvData, 0);

	//Depth buffer
	unsigned int depth;
	glGenRenderbuffers(1, &depth);
	glBindRenderbuffer(GL_RENDERBUFFER, depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer isn't complete" << std::endl;
	}

	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
	glDrawBuffers(4, attachments);

	glViewport(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	int lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.useProgram();

		std::vector<GLfloat> worldspacePositionDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
		std::vector<GLfloat> normalVectorDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);

		std::vector<GLfloat> idDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
		std::vector<GLfloat> uvDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);

		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));

			shader.setUniformMat4("model", lampModel);

			model.meshes[i].draw(shader);

			++lampCounter;
		}
		else {
			shader.setUniformMat4("model", mainObjectModelMatrix);

			model.meshes[i].draw(shader);
		}

		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &worldspacePositionDataBuffer[0]);
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &normalVectorDataBuffer[0]);
		glReadBuffer(GL_COLOR_ATTACHMENT2);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &idDataBuffer[0]);
		glReadBuffer(GL_COLOR_ATTACHMENT3);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &uvDataBuffer[0]);

		model.meshes[i].worldspacePosData = worldspacePositionDataBuffer;
		model.meshes[i].worldspaceNormalData = normalVectorDataBuffer;
		model.meshes[i].idData = idDataBuffer;
		model.meshes[i].uvData = uvDataBuffer;
	}

	/*
	std::cout << normalVectorDataBuffer[1533] << std::endl;
	std::cout << normalVectorDataBuffer[1534] << std::endl;
	std::cout << normalVectorDataBuffer[1535] << std::endl;
	*/
	
	//TODO: Delete textures here if they are not needed anymore
	

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


//Only to be called after the preprocess function
//Do note that this function is incredibly slow - in a final version this functionality would be optimally handled on the GPU but for now, this is sufficient
void Renderer::selectShooter(ObjectModel& model, glm::vec3& shooterRadiance, glm::vec3& shooterWorldspacePos, glm::vec3& shooterWorldspaceNormal, glm::vec2& shooterUV, int& shooterMeshIndex) {
	float maxLuminance = 0.0f;

	//Loop through every mesh
	for (int i = 0; i < model.meshes.size(); ++i) {

		//Loop through every RGB pixel of the radiance map
		for (int j = 0; j < model.meshes[i].radianceData.size(); j += 3) {

			float redValue = model.meshes[i].radianceData[j];
			float greenValue = model.meshes[i].radianceData[j + 1];
			float blueValue = model.meshes[i].radianceData[j + 2];

			//Calculcate pixel luminance
			float currentLuminance = 0.21 * redValue + 0.39 * greenValue + 0.4 * blueValue;

			if (currentLuminance > maxLuminance) {
				float redIDValue = model.meshes[i].idData[j];
				float greenIDValue = model.meshes[i].idData[j + 1];
				float blueIDValue = model.meshes[i].idData[j + 2];

				float idSum = redIDValue + greenIDValue + blueIDValue;

				//Check if pixel maps to a triangle
				if (idSum > 0) {
					shooterRadiance = glm::vec3(model.meshes[i].radianceData[j], model.meshes[i].radianceData[j + 1], model.meshes[i].radianceData[j + 2]);

					shooterWorldspacePos = glm::vec3(model.meshes[i].worldspacePosData[j], model.meshes[i].worldspacePosData[j + 1], model.meshes[i].worldspacePosData[j + 2]);
					shooterWorldspaceNormal = glm::vec3(model.meshes[i].worldspaceNormalData[j], model.meshes[i].worldspaceNormalData[j + 1], model.meshes[i].worldspaceNormalData[j + 2]);

					shooterUV = glm::vec3(model.meshes[i].uvData[j], model.meshes[i].uvData[j + 1], model.meshes[i].uvData[j + 2]);

					shooterMeshIndex = i;


					maxLuminance = currentLuminance;
				}
			}
		}
	}
}

unsigned int Renderer::createVisibilityTexture(ObjectModel& model, ShaderLoader& visibilityShader, glm::mat4& mainObjectModelMatrix, glm::mat4& viewMatrix, unsigned int& resolution) {
	unsigned int visibilityFramebuffer;

	glGenFramebuffers(1, &visibilityFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, visibilityFramebuffer);

	unsigned int visibilityTexture;

	glGenTextures(1, &visibilityTexture);
	glBindTexture(GL_TEXTURE_2D, visibilityTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, resolution, resolution, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, visibilityTexture, 0);

	unsigned int depth;
	glGenRenderbuffers(1, &depth);
	glBindRenderbuffer(GL_RENDERBUFFER, depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, resolution, resolution);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer isn't complete" << std::endl;
	}

	unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	glViewport(0, 0, resolution, resolution);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		visibilityShader.useProgram();

		visibilityShader.setUniformMat4("view", viewMatrix);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));

			visibilityShader.setUniformMat4("model", lampModel);

			model.meshes[i].draw(visibilityShader);

			++lampCounter;
		}
		else {
			visibilityShader.setUniformMat4("model", mainObjectModelMatrix);

			model.meshes[i].draw(visibilityShader);
		}
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteRenderbuffers(1, &depth);
	glDeleteFramebuffers(1, &visibilityFramebuffer);

	return visibilityTexture;
}

//The shooter information has to be bound to the lightmapUpdateShader before calling this function
void Renderer::updateLightmaps(ObjectModel& model, ShaderLoader& lightmapUpdateShader, glm::mat4& mainObjectModelMatrix, glm::mat4& viewMatrix, unsigned int& visibilityTexture) {
	unsigned int lightmapFramebuffer;

	glGenFramebuffers(1, &lightmapFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, lightmapFramebuffer);

	unsigned int newIrradianceTexture;
	unsigned int newRadianceTexture;

	glGenTextures(1, &newIrradianceTexture);
	glBindTexture(GL_TEXTURE_2D, newIrradianceTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, newIrradianceTexture, 0);

	glGenTextures(1, &newRadianceTexture);
	glBindTexture(GL_TEXTURE_2D, newRadianceTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, newRadianceTexture, 0);

	unsigned int depth;
	glGenRenderbuffers(1, &depth);
	glBindRenderbuffer(GL_RENDERBUFFER, depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer isn't complete" << std::endl;
	}

	unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffers(2, attachments);

	glViewport(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	int lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightmapUpdateShader.useProgram();

		std::vector<GLfloat> newIrradianceDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
		std::vector<GLfloat> newRadianceDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);

		lightmapUpdateShader.setUniformMat4("view", viewMatrix);

		//Create textures from the old irradiance and radiance data
		unsigned int irradianceID;
		glGenTextures(1, &irradianceID);

		glBindTexture(GL_TEXTURE_2D, irradianceID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, &model.meshes[i].irradianceData[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);



		unsigned int radianceID;
		glGenTextures(1, &radianceID);

		glBindTexture(GL_TEXTURE_2D, radianceID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, &model.meshes[i].radianceData[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


		//Bind them
		glActiveTexture(GL_TEXTURE0);
		lightmapUpdateShader.setUniformInt("irradianceTexture", 0);
		glBindTexture(GL_TEXTURE_2D, irradianceID);

		glActiveTexture(GL_TEXTURE1);
		lightmapUpdateShader.setUniformInt("radianceTexture", 1);
		glBindTexture(GL_TEXTURE_2D, radianceID);
		
		//Also bind the visibility texture
		glActiveTexture(GL_TEXTURE2);
		lightmapUpdateShader.setUniformInt("visibilityTexture", 2);
		glBindTexture(GL_TEXTURE_2D, visibilityTexture);

		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));

			lightmapUpdateShader.setUniformMat4("model", lampModel);

			model.meshes[i].draw(lightmapUpdateShader);

			++lampCounter;
		}
		else {
			lightmapUpdateShader.setUniformMat4("model", mainObjectModelMatrix);

			model.meshes[i].draw(lightmapUpdateShader);
		}

		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &newIrradianceDataBuffer[0]);
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &newRadianceDataBuffer[0]);

		model.meshes[i].irradianceData = newIrradianceDataBuffer;
		model.meshes[i].radianceData = newRadianceDataBuffer;

		glDeleteTextures(1, &irradianceID);
		glDeleteTextures(1, &radianceID);
	}

	//We'll need to delete the framebuffer and the newIrradiance and newRadiance textures here

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
		addAmbient = true;
	}
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
		addAmbient = false;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		if (preprocessDone == 0) {
			++preprocessDone;
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		glm::vec3 cameraPosition = camera.position;

		lightLocations.push_back(cameraPosition);

		//In the main render loop adds one lamp mesh to the mainObject
		addLampMesh = true;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		doRadiosityIteration = true;
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