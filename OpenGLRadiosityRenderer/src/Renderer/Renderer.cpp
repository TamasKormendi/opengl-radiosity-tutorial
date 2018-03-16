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

bool meshSelectionNeeded = true;
unsigned int shooterIndex = 0;
unsigned int shooterMesh = 0;

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
	ShaderLoader preprocessShader("../src/Shaders/Preprocess.vs", "../src/Shaders/Preprocess.fs");

	ShaderLoader preprocessShaderMultisample("../src/Shaders/PreprocessMultisample.vs", "../src/Shaders/PreprocessMultisample.fs");
	ShaderLoader preprocessResolveShader("../src/Shaders/ShooterMeshSelection.vs", "../src/Shaders/PreprocessResolve.fs");

	ShaderLoader shooterMeshSelectionShader("../src/Shaders/ShooterMeshSelection.vs", "../src/Shaders/ShooterMeshSelection.fs");

	//ShaderLoader visibilityTextureShader("../src/Shaders/HemisphereVisibilityTexture.vs", "../src/Shaders/HemisphereVisibilityTexture.fs");
	ShaderLoader lightmapUpdateShader("../src/Shaders/LightmapUpdate.vs", "../src/Shaders/LightmapUpdate.fs");
	ShaderLoader lightmapUpdateShaderMultisample("../src/Shaders/LightmapUpdateMultisample.vs", "../src/Shaders/LightmapUpdateMultisample.fs");
	ShaderLoader lightmapResolveShader("../src/Shaders/ShooterMeshSelection.vs", "../src/Shaders/LightmapResolve.fs");

	ShaderLoader finalRenderShader("../src/Shaders/FinalRender.vs", "../src/Shaders/FinalRender.fs");
	ShaderLoader framebufferDebugShader("../src/Shaders/FramebufferDebug.vs", "../src/Shaders/FramebufferDebug.fs");

	ShaderLoader hemicubeVisibilityShader("../src/Shaders/HemicubeVisibilityTexture.vs", "../src/Shaders/HemicubeVisibilityTexture.fs");

	ObjectModel mainModel(objectFilepath, false);
	ObjectModel lampModel("../assets/lamp.obj", true);

	int frameCounter = 0;
	double fpsTimeCounter = glfwGetTime();

	int iterationNumber = 0;


	float shooterMeshSelectionQuadVertices[] = {
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
		1.0f,  1.0f,  1.0f, 1.0f
	};

	unsigned int shooterMeshSelectionQuadVAO, shooterMeshSelectionQuadVBO;
	glGenVertexArrays(1, &shooterMeshSelectionQuadVAO);
	glGenBuffers(1, &shooterMeshSelectionQuadVBO);
	glBindVertexArray(shooterMeshSelectionQuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, shooterMeshSelectionQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(shooterMeshSelectionQuadVertices), &shooterMeshSelectionQuadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	//Based on https://learnopengl.com/In-Practice/Debugging
	float quadVertices[] = {
		// positions   // texCoords
		0.0f,  1.0f,  0.0f, 1.0f,
		0.0f, 0.0f,  0.0f, 0.0f,
		1.0f, 0.0f,  1.0f, 0.0f,

		0.0f,  1.0f,  0.0f, 1.0f,
		1.0f, 0.0f,  1.0f, 0.0f,
		1.0f,  1.0f,  1.0f, 1.0f
	};

	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	bool debugInitialised = false;
	unsigned int debugTexture = 0;
	std::vector<unsigned int> debugTextures = std::vector<unsigned int>();


	//RENDER LOOP STARTS HERE
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

			//preprocess(mainModel, preprocessShader, model);
			preprocessMultisample(mainModel, preprocessShaderMultisample, model, preprocessResolveShader, shooterMeshSelectionQuadVAO);

			/*std::cout << mainModel.meshes[7].uvData[1533] << std::endl;
			std::cout << mainModel.meshes[7].uvData[1534] << std::endl;
			std::cout << mainModel.meshes[7].uvData[1535] << std::endl;*/

			//This is needed so preprocess can only be done once

			std::cout << "Preprocess done" << std::endl;
			++preprocessDone;
		}

		glm::vec3 shooterRadiance;
		glm::vec3 shooterWorldspacePos;
		glm::vec3 shooterWorldspaceNormal;
		glm::vec2 shooterUV;
		int shooterMeshIndex;


		//Radiosity iteration section
		if (doRadiosityIteration) {
			
			//for (int i = 0; i < 100; ++i) {

			if (debugInitialised) {
				//glDeleteTextures(1, &debugTexture);

				for (unsigned int debugTexture : debugTextures) {
					glDeleteTextures(1, &debugTexture);
				}
			}



			if (meshSelectionNeeded) {
				shooterMesh = selectShooterMesh(mainModel, shooterMeshSelectionShader, shooterMeshSelectionQuadVAO);

				meshSelectionNeeded = false;
			}

			//while (!meshSelectionNeeded) {
				

				selectMeshBasedShooter(mainModel, shooterRadiance, shooterWorldspacePos, shooterWorldspaceNormal, shooterUV, shooterMesh);



				//selectShooter(mainModel, shooterRadiance, shooterWorldspacePos, shooterWorldspaceNormal, shooterUV, shooterMeshIndex);


				
				std::cout << shooterWorldspaceNormal.x << std::endl;
				std::cout << shooterWorldspaceNormal.y << std::endl;
				std::cout << shooterWorldspaceNormal.z << std::endl;
				


				//TODO: The upVector most likely fails if we have a normal along +y or -y
				/*
				glm::vec3 upVec = glm::vec3(shooterWorldspaceNormal.x, shooterWorldspaceNormal.z, -shooterWorldspaceNormal.y);

				if (upVec.y == 0.0f && upVec.z == 0.0f) {
					upVec.y = 1.0f;
				}

				glm::mat4 shooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + shooterWorldspaceNormal, upVec);
				*/

				std::vector<glm::mat4> shooterViews;

				unsigned int visibilityTextureSize = 1024;

				glm::mat4 shooterProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

				hemicubeVisibilityShader.useProgram();
				hemicubeVisibilityShader.setUniformMat4("projection", shooterProj);

				//unsigned int visibilityTexture = createVisibilityTexture(mainModel, hemicubeVisibilityShader, model, shooterView, visibilityTextureSize);

				std::vector<unsigned int> visibilityTextures = createHemicubeTextures(mainModel, hemicubeVisibilityShader, model, shooterViews, visibilityTextureSize, shooterWorldspacePos, shooterWorldspaceNormal);

				debugTextures = visibilityTextures;

				debugTexture = visibilityTextures[1];
				debugInitialised = true;

				/*
				std::cout << "Shooter red " << shooterRadiance.x << std::endl;
				std::cout << "Shooter green " << shooterRadiance.y << std::endl;
				std::cout << "Shooter blue " << shooterRadiance.z << std::endl;
				*/

				//Scale down the shooter's strength - COMMENTED OUT FOR NOW
				//shooterRadiance = glm::vec3(1 * shooterRadiance.x / (::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE),
				//	1 * shooterRadiance.y / (::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE),
				//	1 * shooterRadiance.z / (::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE));


				/*
				//This is an incredibly important section since this needs to be changed between the MSAA and non-MSAA rendering
				lightmapUpdateShader.useProgram();
				lightmapUpdateShader.setUniformVec3("shooterRadiance", shooterRadiance);
				lightmapUpdateShader.setUniformVec3("shooterWorldspacePos", shooterWorldspacePos);
				lightmapUpdateShader.setUniformVec3("shooterWorldspaceNormal", shooterWorldspaceNormal);
				lightmapUpdateShader.setUniformVec2("shooterUV", shooterUV);
				//lightmapUpdateShader.setUniformMat4("projection", shooterProj);
				updateLightmaps(mainModel, lightmapUpdateShader, model, shooterViews, visibilityTextures);
				*/

				
				lightmapUpdateShaderMultisample.useProgram();
				lightmapUpdateShaderMultisample.setUniformVec3("shooterRadiance", shooterRadiance);
				lightmapUpdateShaderMultisample.setUniformVec3("shooterWorldspacePos", shooterWorldspacePos);
				lightmapUpdateShaderMultisample.setUniformVec3("shooterWorldspaceNormal", shooterWorldspaceNormal);
				lightmapUpdateShaderMultisample.setUniformVec2("shooterUV", shooterUV);
				updateLightmapsMultisample(mainModel, lightmapUpdateShaderMultisample, model, shooterViews, visibilityTextures, lightmapResolveShader, shooterMeshSelectionQuadVAO);
				

				//glDeleteTextures(1, &visibilityTexture);

				for (unsigned int debugTexture : debugTextures) {
					glDeleteTextures(1, &debugTexture);
				}

				std::cout << iterationNumber << std::endl;

				++iterationNumber;

				//std::cout << i << std::endl;

			//}
			//}
			//Uncomment this to resume manual iteration
			//doRadiosityIteration = false;
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

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glActiveTexture(GL_TEXTURE0);
				finalRenderShader.setUniformInt("irradianceTexture", 0);
				glBindTexture(GL_TEXTURE_2D, irradianceID);
				

				mainModel.meshes[i].draw(finalRenderShader);

				glDeleteTextures(1, &irradianceID);

				++lampCounter;
			}
			else {
				finalRenderShader.useProgram();

				finalRenderShader.setUniformMat4("model", model);

				
				unsigned int irradianceID;
				glGenTextures(1, &irradianceID);

				glBindTexture(GL_TEXTURE_2D, irradianceID);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, &mainModel.meshes[i].irradianceData[0]);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glActiveTexture(GL_TEXTURE0);
				finalRenderShader.setUniformInt("irradianceTexture", 0);
				glBindTexture(GL_TEXTURE_2D, irradianceID);
				

				mainModel.meshes[i].draw(finalRenderShader);

				glDeleteTextures(1, &irradianceID);
			}
		}

		if (false) {
			displayFramebufferTexture(framebufferDebugShader, quadVAO, debugTexture);
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

	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
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

		for (unsigned int j = 0; j < model.meshes[i].idData.size(); j += 3) {
			float redIDValue = model.meshes[i].idData[j];
			float greenIDValue = model.meshes[i].idData[j + 1];
			float blueIDValue = model.meshes[i].idData[j + 2];

			float idSum = redIDValue + greenIDValue + blueIDValue;

			if (idSum > 0) {
				model.meshes[i].texturespaceShooterIndices.push_back(j);
			}
		}
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

//This function draws a 1x1 mipmap of every mesh into a screen-aligned quad, their depth weighted with the mipmap value and reads back the resulting 1x1 texture
unsigned int Renderer::selectShooterMesh(ObjectModel& model, ShaderLoader& shooterMeshSelectionShader, unsigned int& screenAlignedQuadVAO) {
	unsigned int shooterSelectionFramebuffer;

	glGenFramebuffers(1, &shooterSelectionFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, shooterSelectionFramebuffer);

	unsigned int meshIDTexture;

	glGenTextures(1, &meshIDTexture);
	glBindTexture(GL_TEXTURE_2D, meshIDTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 1, 1, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, meshIDTexture, 0);

	unsigned int depth;
	glGenRenderbuffers(1, &depth);
	glBindRenderbuffer(GL_RENDERBUFFER, depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1, 1);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer isn't complete" << std::endl;
	}

	unsigned int attachments[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, attachments);

	glViewport(0, 0, 1, 1);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shooterMeshSelectionShader.useProgram();
	glBindVertexArray(screenAlignedQuadVAO);


	//unsigned int meshID = 0;
	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		unsigned int meshID = i;

		float redValue = meshID % 100;

		int greenRemainingValue = (int)meshID / 100;

		float greenValue = greenRemainingValue % 100;

		int blueRemainingValue = (int)greenRemainingValue / 100;

		float blueValue = blueRemainingValue % 100;

		redValue = redValue / 100.0f;
		greenValue = greenValue / 100.0f;
		blueValue = blueValue / 100.0f;

		glm::vec3 meshIDVector = glm::vec3(redValue, greenValue, blueValue);

		shooterMeshSelectionShader.setUniformVec3("meshID", meshIDVector);

		unsigned int mipmappedRadianceTexture;

		glGenTextures(1, &mipmappedRadianceTexture);
		glBindTexture(GL_TEXTURE_2D, mipmappedRadianceTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, &model.meshes[i].radianceData[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glActiveTexture(GL_TEXTURE0);
		shooterMeshSelectionShader.setUniformInt("mipmappedRadianceTexture", 0);
		glBindTexture(GL_TEXTURE_2D, mipmappedRadianceTexture);


		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDeleteTextures(1, &mipmappedRadianceTexture);
	}

	std::vector<GLfloat> shooterMeshID(1 * 1 * 3);

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, 1, 1, GL_RGB, GL_FLOAT, &shooterMeshID[0]);

	std::cout << "Shooter mesh ID R: " << shooterMeshID[0] << std::endl;
	std::cout << "Shooter mesh ID G: " << shooterMeshID[1] << std::endl;
	std::cout << "Shooter mesh ID B: " << shooterMeshID[2] << std::endl;

	unsigned int chosenShooterMeshID = 0;

	//The rounding is needed because the program otherwise thought 0.59 * 100 is 58...yeah
	chosenShooterMeshID += std::round(100 * 100 * 100 * shooterMeshID[2]);
	chosenShooterMeshID += std::round(100 * 100 * shooterMeshID[1]);
	chosenShooterMeshID += std::round(100 * shooterMeshID[0]);

	std::cout << "Chosen shooter mesh ID is: " << chosenShooterMeshID << std::endl;

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	//Dummy return for now, also need to delete whatever we need to delete before this
	glDeleteFramebuffers(1, &shooterSelectionFramebuffer);
	glDeleteRenderbuffers(1, &depth);
	glDeleteTextures(1, &meshIDTexture);

	return chosenShooterMeshID;
}

void Renderer::selectMeshBasedShooter(ObjectModel& model, glm::vec3& shooterRadiance, glm::vec3& shooterWorldspacePos, glm::vec3& shooterWorldspaceNormal, glm::vec2& shooterUV, unsigned int& shooterMeshIndex) {
	unsigned int texelIndex = model.meshes[shooterMeshIndex].texturespaceShooterIndices[shooterIndex];

	shooterRadiance = glm::vec3(model.meshes[shooterMeshIndex].radianceData[texelIndex] / model.meshes[shooterMeshIndex].texturespaceShooterIndices.size(),
								model.meshes[shooterMeshIndex].radianceData[texelIndex + 1] / model.meshes[shooterMeshIndex].texturespaceShooterIndices.size(),
								model.meshes[shooterMeshIndex].radianceData[texelIndex + 2] / model.meshes[shooterMeshIndex].texturespaceShooterIndices.size());

	shooterWorldspacePos = glm::vec3(model.meshes[shooterMeshIndex].worldspacePosData[texelIndex], model.meshes[shooterMeshIndex].worldspacePosData[texelIndex + 1], model.meshes[shooterMeshIndex].worldspacePosData[texelIndex + 2]);

	shooterWorldspaceNormal = glm::vec3(model.meshes[shooterMeshIndex].worldspaceNormalData[texelIndex], model.meshes[shooterMeshIndex].worldspaceNormalData[texelIndex + 1], model.meshes[shooterMeshIndex].worldspaceNormalData[texelIndex + 2]);

	shooterUV = glm::vec3(model.meshes[shooterMeshIndex].uvData[texelIndex], model.meshes[shooterMeshIndex].uvData[texelIndex + 1], model.meshes[shooterMeshIndex].uvData[texelIndex + 2]);


	model.meshes[shooterMeshIndex].radianceData[texelIndex] = 0.0f;
	model.meshes[shooterMeshIndex].radianceData[texelIndex + 1] = 0.0f;
	model.meshes[shooterMeshIndex].radianceData[texelIndex + 2] = 0.0f;

	
	//std::cout << "Processing shooter number " << shooterIndex << " out of " << model.meshes[shooterMeshIndex].texturespaceShooterIndices.size();

	if (shooterIndex >= model.meshes[shooterMeshIndex].texturespaceShooterIndices.size() - 1) {
		meshSelectionNeeded = true;

		shooterIndex = 0;
	}
	else {
		++shooterIndex;
	}


}

//Only to be called after the preprocess function
//Do note that this function is incredibly slow - in a final version this functionality would be optimally handled on the GPU but for now, this is sufficient
//TODO: Change this to per-mesh selection followed by linear shooter processing (the former through mipmapping)
void Renderer::selectShooter(ObjectModel& model, glm::vec3& shooterRadiance, glm::vec3& shooterWorldspacePos, glm::vec3& shooterWorldspaceNormal, glm::vec2& shooterUV, int& shooterMeshIndex) {
	float maxLuminance = 0.0f;


	int meshIndex = 0;
	int pixelIndex = 0;

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

				float yUpTest = model.meshes[i].worldspaceNormalData[j + 1];

				//Check if pixel maps to a triangle
				if (idSum > 0) {
					shooterRadiance = glm::vec3(model.meshes[i].radianceData[j], model.meshes[i].radianceData[j + 1], model.meshes[i].radianceData[j + 2]);

					shooterWorldspacePos = glm::vec3(model.meshes[i].worldspacePosData[j], model.meshes[i].worldspacePosData[j + 1], model.meshes[i].worldspacePosData[j + 2]);
					shooterWorldspaceNormal = glm::vec3(model.meshes[i].worldspaceNormalData[j], model.meshes[i].worldspaceNormalData[j + 1], model.meshes[i].worldspaceNormalData[j + 2]);

					shooterUV = glm::vec3(model.meshes[i].uvData[j], model.meshes[i].uvData[j + 1], model.meshes[i].uvData[j + 2]);

					shooterMeshIndex = i;


					maxLuminance = currentLuminance;

					meshIndex = i;
					pixelIndex = j;
				}
			}
		}
	}

	model.meshes[meshIndex].radianceData[pixelIndex] = 0.0f;
	model.meshes[meshIndex].radianceData[pixelIndex + 1] = 0.0f;
	model.meshes[meshIndex].radianceData[pixelIndex + 2] = 0.0f;
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

//TODO: This function could definitely use some refactoring down the line, but it does work
std::vector<unsigned int> Renderer::createHemicubeTextures(ObjectModel& model,
	ShaderLoader& hemicubeShader,
	glm::mat4& mainObjectModelMatrix,
	std::vector<glm::mat4>& viewMatrices,
	unsigned int& resolution,
	glm::vec3& shooterWorldspacePos,
	glm::vec3& shooterWorldspaceNormal) {

	float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 normalisedShooterNormal = glm::normalize(shooterWorldspaceNormal);

	if (normalisedShooterNormal.x == 0.0f && normalisedShooterNormal.y == 1.0f && normalisedShooterNormal.z == 0.0f) {
		worldUp = glm::vec3(0.0f, 0.0f, -1.0f);
	}
	else if (normalisedShooterNormal.x == 0.0f && normalisedShooterNormal.y == -1.0f && normalisedShooterNormal.z == 0.0f) {
		worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
	}

	glm::vec3 hemicubeRight = glm::normalize(glm::cross(shooterWorldspaceNormal, worldUp));
	glm::vec3 hemicubeUp = glm::normalize(glm::cross(hemicubeRight, shooterWorldspaceNormal));


	/*
	glm::vec3 upVec = glm::vec3(shooterWorldspaceNormal.x, shooterWorldspaceNormal.z, -shooterWorldspaceNormal.y);

	if (upVec.y == 0.0f && upVec.z == 0.0f) {
		upVec.y = 1.0f;
	}
	*/


	glm::mat4 frontShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + shooterWorldspaceNormal, hemicubeUp);

	glm::mat4 leftShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + (-hemicubeRight), hemicubeUp);
	glm::mat4 rightShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + hemicubeRight, hemicubeUp);

	glm::mat4 upShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + hemicubeUp, -normalisedShooterNormal);
	glm::mat4 downShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + (-hemicubeUp), normalisedShooterNormal);


	//glm::mat4 leftShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + glm::vec3(shooterWorldspaceNormal.z, shooterWorldspaceNormal.y, -shooterWorldspaceNormal.x), upVec);
	//glm::mat4 rightShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + glm::vec3(-shooterWorldspaceNormal.z, shooterWorldspaceNormal.y, shooterWorldspaceNormal.x), upVec);

	//glm::vec3 upUpVec = glm::vec3(upVec.x, upVec.z, -upVec.y);
	//glm

	//This would actually shoot up
	//glm::mat4 leftShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));

	viewMatrices.push_back(frontShooterView);

	viewMatrices.push_back(leftShooterView);
	viewMatrices.push_back(rightShooterView);

	viewMatrices.push_back(upShooterView);
	viewMatrices.push_back(downShooterView);


	unsigned int hemicubeFrontFramebuffer;
	glGenFramebuffers(1, &hemicubeFrontFramebuffer);

	unsigned int frontDepthMap;
	glGenTextures(1, &frontDepthMap);
	glBindTexture(GL_TEXTURE_2D, frontDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//This part is needed to avoid light bleeding by oversampling (so sampling outside the depth texture)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFrontFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, frontDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glViewport(0, 0, resolution, resolution);

	//Need to get rid of this when/if we cut off half of the depth maps
	glClear(GL_DEPTH_BUFFER_BIT);

	glm::mat4 shooterProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

	int lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		hemicubeShader.useProgram();


		hemicubeShader.setUniformMat4("projection", shooterProj);

		hemicubeShader.setUniformMat4("view", frontShooterView);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));

			hemicubeShader.setUniformMat4("model", lampModel);

			model.meshes[i].draw(hemicubeShader);

			++lampCounter;
		}
		else {
			hemicubeShader.setUniformMat4("model", mainObjectModelMatrix);

			model.meshes[i].draw(hemicubeShader);
		}
	}

	unsigned int leftDepthMap;
	glGenTextures(1, &leftDepthMap);
	glBindTexture(GL_TEXTURE_2D, leftDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//This part is needed to avoid light bleeding by oversampling (so sampling outside the depth texture)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFrontFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, leftDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glViewport(0, 0, resolution, resolution);

	glClear(GL_DEPTH_BUFFER_BIT);

	lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		hemicubeShader.useProgram();


		hemicubeShader.setUniformMat4("projection", shooterProj);

		hemicubeShader.setUniformMat4("view", leftShooterView);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));

			hemicubeShader.setUniformMat4("model", lampModel);

			model.meshes[i].draw(hemicubeShader);

			++lampCounter;
		}
		else {
			hemicubeShader.setUniformMat4("model", mainObjectModelMatrix);

			model.meshes[i].draw(hemicubeShader);
		}
	}

	unsigned int rightDepthMap;
	glGenTextures(1, &rightDepthMap);
	glBindTexture(GL_TEXTURE_2D, rightDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//This part is needed to avoid light bleeding by oversampling (so sampling outside the depth texture)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFrontFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rightDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glViewport(0, 0, resolution, resolution);

	glClear(GL_DEPTH_BUFFER_BIT);

	lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		hemicubeShader.useProgram();


		hemicubeShader.setUniformMat4("projection", shooterProj);

		hemicubeShader.setUniformMat4("view", rightShooterView);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));

			hemicubeShader.setUniformMat4("model", lampModel);

			model.meshes[i].draw(hemicubeShader);

			++lampCounter;
		}
		else {
			hemicubeShader.setUniformMat4("model", mainObjectModelMatrix);

			model.meshes[i].draw(hemicubeShader);
		}
	}

	unsigned int upDepthMap;
	glGenTextures(1, &upDepthMap);
	glBindTexture(GL_TEXTURE_2D, upDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//This part is needed to avoid light bleeding by oversampling (so sampling outside the depth texture)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFrontFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, upDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glViewport(0, 0, resolution, resolution);

	glClear(GL_DEPTH_BUFFER_BIT);

	lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		hemicubeShader.useProgram();


		hemicubeShader.setUniformMat4("projection", shooterProj);

		hemicubeShader.setUniformMat4("view", upShooterView);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));

			hemicubeShader.setUniformMat4("model", lampModel);

			model.meshes[i].draw(hemicubeShader);

			++lampCounter;
		}
		else {
			hemicubeShader.setUniformMat4("model", mainObjectModelMatrix);

			model.meshes[i].draw(hemicubeShader);
		}
	}

	unsigned int downDepthMap;
	glGenTextures(1, &downDepthMap);
	glBindTexture(GL_TEXTURE_2D, downDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//This part is needed to avoid light bleeding by oversampling (so sampling outside the depth texture)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFrontFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, downDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glViewport(0, 0, resolution, resolution);

	glClear(GL_DEPTH_BUFFER_BIT);

	lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		hemicubeShader.useProgram();


		hemicubeShader.setUniformMat4("projection", shooterProj);

		hemicubeShader.setUniformMat4("view", downShooterView);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));

			hemicubeShader.setUniformMat4("model", lampModel);

			model.meshes[i].draw(hemicubeShader);

			++lampCounter;
		}
		else {
			hemicubeShader.setUniformMat4("model", mainObjectModelMatrix);

			model.meshes[i].draw(hemicubeShader);
		}
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	std::vector<unsigned int> depthTextures;

	depthTextures.push_back(frontDepthMap);

	depthTextures.push_back(leftDepthMap);
	depthTextures.push_back(rightDepthMap);

	depthTextures.push_back(upDepthMap);
	depthTextures.push_back(downDepthMap);

	glDeleteFramebuffers(1, &hemicubeFrontFramebuffer);


	return depthTextures;
}

//The shooter information has to be bound to the lightmapUpdateShader before calling this function
void Renderer::updateLightmaps(ObjectModel& model, ShaderLoader& lightmapUpdateShader, glm::mat4& mainObjectModelMatrix, std::vector<glm::mat4>& viewMatrices, std::vector<unsigned int>& visibilityTextures) {
	//std::cout << " Start: " << glfwGetTime() << std::endl;

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

	//std::cout << "Loop start: " << glfwGetTime() << std::endl;
	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightmapUpdateShader.useProgram();

		std::vector<GLfloat> newIrradianceDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
		std::vector<GLfloat> newRadianceDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);

		glm::mat4 shooterProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

		lightmapUpdateShader.useProgram();
		lightmapUpdateShader.setUniformMat4("projection", shooterProj);

		lightmapUpdateShader.setUniformMat4("view", viewMatrices[0]);

		lightmapUpdateShader.setUniformMat4("leftView", viewMatrices[1]);
		lightmapUpdateShader.setUniformMat4("rightView", viewMatrices[2]);

		lightmapUpdateShader.setUniformMat4("upView", viewMatrices[3]);
		lightmapUpdateShader.setUniformMat4("downView", viewMatrices[4]);


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
		
		//Also bind the visibility textures, they're a bit scattered for now
		glActiveTexture(GL_TEXTURE2);
		lightmapUpdateShader.setUniformInt("visibilityTexture", 2);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[0]);

		glActiveTexture(GL_TEXTURE10);
		lightmapUpdateShader.setUniformInt("leftVisibilityTexture", 10);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[1]);

		glActiveTexture(GL_TEXTURE11);
		lightmapUpdateShader.setUniformInt("rightVisibilityTexture", 11);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[2]);

		glActiveTexture(GL_TEXTURE12);
		lightmapUpdateShader.setUniformInt("upVisibilityTexture", 12);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[3]);

		glActiveTexture(GL_TEXTURE13);
		lightmapUpdateShader.setUniformInt("downVisibilityTexture", 13);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[4]);

		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));

			lightmapUpdateShader.setUniformMat4("model", lampModel);
			lightmapUpdateShader.setUniformBool("isLamp", true);

			model.meshes[i].draw(lightmapUpdateShader);

			++lampCounter;
		}
		else {
			lightmapUpdateShader.setUniformMat4("model", mainObjectModelMatrix);

			lightmapUpdateShader.setUniformBool("isLamp", false);

			model.meshes[i].draw(lightmapUpdateShader);
		}

		//std::cout << "Readback Start: " << glfwGetTime() << std::endl;
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &newIrradianceDataBuffer[0]);
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &newRadianceDataBuffer[0]);
		//std::cout << "Readback end: " << glfwGetTime() << std::endl;

		//std::cout << "Copy Start: " << glfwGetTime() << std::endl;
		model.meshes[i].irradianceData = newIrradianceDataBuffer;
		model.meshes[i].radianceData = newRadianceDataBuffer;
		//std::cout << "Copy end: " << glfwGetTime() << std::endl;

		glDeleteTextures(1, &irradianceID);
		glDeleteTextures(1, &radianceID);
	}
	//std::cout << "Loop end: " << glfwGetTime() << std::endl;

	//We'll need to delete the framebuffer and the newIrradiance and newRadiance textures here

	glDeleteTextures(1, &newIrradianceTexture);
	glDeleteTextures(1, &newRadianceTexture);
	glDeleteRenderbuffers(1, &depth);
	glDeleteFramebuffers(1, &lightmapFramebuffer);

	//glReadBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//std::cout << "Function end: " << glfwGetTime() << std::endl;
}

void Renderer::preprocessMultisample(ObjectModel& model, ShaderLoader& shader, glm::mat4& mainObjectModelMatrix, ShaderLoader& resolveShader, unsigned int& screenAlignedQuadVAO) {
	unsigned int intermediateFramebuffer;

	glGenFramebuffers(1, &intermediateFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateFramebuffer);

	unsigned int downsampledPosData;
	unsigned int downsampledNormalData;
	unsigned int downsampledIDData;
	unsigned int downsampledUVData;

	glGenTextures(1, &downsampledPosData);
	glBindTexture(GL_TEXTURE_2D, downsampledPosData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, downsampledPosData, 0);

	glGenTextures(1, &downsampledNormalData);
	glBindTexture(GL_TEXTURE_2D, downsampledNormalData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, downsampledNormalData, 0);

	glGenTextures(1, &downsampledIDData);
	glBindTexture(GL_TEXTURE_2D, downsampledIDData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, downsampledIDData, 0);

	glGenTextures(1, &downsampledUVData);
	glBindTexture(GL_TEXTURE_2D, downsampledUVData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, downsampledUVData, 0);

	unsigned int resolveAttachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, resolveAttachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Intermediate Framebuffer isn't complete" << std::endl;
	}

	unsigned int preprocessFramebuffer;

	glGenFramebuffers(1, &preprocessFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, preprocessFramebuffer);

	unsigned int worldspacePosData;
	unsigned int worldspaceNormalData;

	unsigned int idData;
	unsigned int uvData;

	unsigned int samples = 8;

	//Create texture to hold worldspace-position data
	glGenTextures(1, &worldspacePosData);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, worldspacePosData);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_TRUE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, worldspacePosData, 0);

	//Create texture to hold worldspace-normal data
	glGenTextures(1, &worldspaceNormalData);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, worldspaceNormalData);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_TRUE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, worldspaceNormalData, 0);

	//Create texture to hold triangle IDs in texturespace
	glGenTextures(1, &idData);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, idData);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_TRUE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D_MULTISAMPLE, idData, 0);

	//Create texture to hold UV-coordinate data in texturespace
	glGenTextures(1, &uvData);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, uvData);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_TRUE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D_MULTISAMPLE, uvData, 0);

	//Depth buffer
	unsigned int depth;
	glGenRenderbuffers(1, &depth);
	glBindRenderbuffer(GL_RENDERBUFFER, depth);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer isn't complete" << std::endl;
	}

	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	glViewport(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	int lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, preprocessFramebuffer);

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

		glBindFramebuffer(GL_FRAMEBUFFER, intermediateFramebuffer);
		glClear(GL_COLOR_BUFFER_BIT);
		//glDrawBuffer(GL_COLOR_ATTACHMENT1);

		//glBlitFramebuffer(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_COLOR_BUFFER_BIT, GL_NEAREST);


		resolveShader.useProgram();
		glBindVertexArray(screenAlignedQuadVAO);

		glActiveTexture(GL_TEXTURE0);
		resolveShader.setUniformInt("multisampledPosTexture", 0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, worldspacePosData);

		glActiveTexture(GL_TEXTURE1);
		resolveShader.setUniformInt("multisampledNormalTexture", 1);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, worldspaceNormalData);

		glActiveTexture(GL_TEXTURE2);
		resolveShader.setUniformInt("multisampledIDTexture", 2);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, idData);

		glActiveTexture(GL_TEXTURE3);
		resolveShader.setUniformInt("multisampledUVTexture", 3);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, uvData);


		glDrawArrays(GL_TRIANGLES, 0, 6);



		glBindFramebuffer(GL_FRAMEBUFFER, intermediateFramebuffer);

		/*
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &normalVectorDataBuffer[0]);
		*/

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

		for (unsigned int j = 0; j < model.meshes[i].idData.size(); j += 3) {
			float redIDValue = model.meshes[i].idData[j];
			float greenIDValue = model.meshes[i].idData[j + 1];
			float blueIDValue = model.meshes[i].idData[j + 2];

			float idSum = redIDValue + greenIDValue + blueIDValue;

			if (idSum > 0) {
				model.meshes[i].texturespaceShooterIndices.push_back(j);
			}
		}
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

void Renderer::updateLightmapsMultisample(ObjectModel& model, ShaderLoader& lightmapUpdateShader, glm::mat4& mainObjectModelMatrix, std::vector<glm::mat4>& viewMatrices, std::vector<unsigned int>& visibilityTextures, ShaderLoader& resolveShader, unsigned int& screenAlignedQuadVAO) {
	//std::cout << " Start: " << glfwGetTime() << std::endl;

	

	unsigned int intermediateFramebuffer;

	glGenFramebuffers(1, &intermediateFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateFramebuffer);

	unsigned int downsampledNewIrradianceTexture;
	unsigned int downsampledNewRadianceTexture;

	glGenTextures(1, &downsampledNewIrradianceTexture);
	glBindTexture(GL_TEXTURE_2D, downsampledNewIrradianceTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, downsampledNewIrradianceTexture, 0);

	glGenTextures(1, &downsampledNewRadianceTexture);
	glBindTexture(GL_TEXTURE_2D, downsampledNewRadianceTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, downsampledNewRadianceTexture, 0);

	unsigned int resolveAttachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffers(2, resolveAttachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Intermediate Framebuffer isn't complete" << std::endl;
	}

	unsigned int lightmapFramebuffer;

	glGenFramebuffers(1, &lightmapFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, lightmapFramebuffer);

	unsigned int newIrradianceTexture;
	unsigned int newRadianceTexture;

	unsigned int samples = 8;

	glGenTextures(1, &newIrradianceTexture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, newIrradianceTexture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_TRUE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, newIrradianceTexture, 0);

	glGenTextures(1, &newRadianceTexture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, newRadianceTexture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_TRUE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, newRadianceTexture, 0);

	unsigned int depth;
	glGenRenderbuffers(1, &depth);
	glBindRenderbuffer(GL_RENDERBUFFER, depth);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer isn't complete" << std::endl;
	}

	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	glDrawBuffers(2, attachments);

	glViewport(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	int lampCounter = 0;

	//std::cout << "Loop start: " << glfwGetTime() << std::endl;
	for (unsigned int i = 0; i < model.meshes.size(); ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, lightmapFramebuffer);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightmapUpdateShader.useProgram();

		std::vector<GLfloat> newIrradianceDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
		std::vector<GLfloat> newRadianceDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);

		glm::mat4 shooterProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

		lightmapUpdateShader.useProgram();
		lightmapUpdateShader.setUniformMat4("projection", shooterProj);

		lightmapUpdateShader.setUniformMat4("view", viewMatrices[0]);

		lightmapUpdateShader.setUniformMat4("leftView", viewMatrices[1]);
		lightmapUpdateShader.setUniformMat4("rightView", viewMatrices[2]);

		lightmapUpdateShader.setUniformMat4("upView", viewMatrices[3]);
		lightmapUpdateShader.setUniformMat4("downView", viewMatrices[4]);


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

		//Also bind the visibility textures, they're a bit scattered for now
		glActiveTexture(GL_TEXTURE2);
		lightmapUpdateShader.setUniformInt("visibilityTexture", 2);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[0]);

		glActiveTexture(GL_TEXTURE10);
		lightmapUpdateShader.setUniformInt("leftVisibilityTexture", 10);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[1]);

		glActiveTexture(GL_TEXTURE11);
		lightmapUpdateShader.setUniformInt("rightVisibilityTexture", 11);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[2]);

		glActiveTexture(GL_TEXTURE12);
		lightmapUpdateShader.setUniformInt("upVisibilityTexture", 12);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[3]);

		glActiveTexture(GL_TEXTURE13);
		lightmapUpdateShader.setUniformInt("downVisibilityTexture", 13);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[4]);

		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(0.05f));

			lightmapUpdateShader.setUniformMat4("model", lampModel);
			lightmapUpdateShader.setUniformBool("isLamp", true);

			model.meshes[i].draw(lightmapUpdateShader);

			++lampCounter;
		}
		else {
			lightmapUpdateShader.setUniformMat4("model", mainObjectModelMatrix);

			lightmapUpdateShader.setUniformBool("isLamp", false);

			model.meshes[i].draw(lightmapUpdateShader);
		}

		/*
		glBindFramebuffer(GL_READ_FRAMEBUFFER, lightmapFramebuffer);
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFramebuffer);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		glBlitFramebuffer(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, lightmapFramebuffer);
		glReadBuffer(GL_COLOR_ATTACHMENT1);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFramebuffer);
		glDrawBuffer(GL_COLOR_ATTACHMENT1);

		glBlitFramebuffer(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		*/

		
		glBindFramebuffer(GL_FRAMEBUFFER, intermediateFramebuffer);
		glClear(GL_COLOR_BUFFER_BIT);

		resolveShader.useProgram();
		glBindVertexArray(screenAlignedQuadVAO);

		glActiveTexture(GL_TEXTURE0);
		resolveShader.setUniformInt("multisampledNewIrradianceTexture", 0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, newIrradianceTexture);

		glActiveTexture(GL_TEXTURE1);
		resolveShader.setUniformInt("multisampledNewRadianceTexture", 1);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, newRadianceTexture);


		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindFramebuffer(GL_FRAMEBUFFER, intermediateFramebuffer);


		//std::cout << "Readback Start: " << glfwGetTime() << std::endl;
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &newIrradianceDataBuffer[0]);
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &newRadianceDataBuffer[0]);
		//std::cout << "Readback end: " << glfwGetTime() << std::endl;

		//std::cout << "Copy Start: " << glfwGetTime() << std::endl;
		model.meshes[i].irradianceData = newIrradianceDataBuffer;
		model.meshes[i].radianceData = newRadianceDataBuffer;
		//std::cout << "Copy end: " << glfwGetTime() << std::endl;

		glDeleteTextures(1, &irradianceID);
		glDeleteTextures(1, &radianceID);
	}
	//std::cout << "Loop end: " << glfwGetTime() << std::endl;

	//We'll need to delete the framebuffer and the newIrradiance and newRadiance textures here

	glDeleteTextures(1, &newIrradianceTexture);
	glDeleteTextures(1, &newRadianceTexture);

	glDeleteTextures(1, &downsampledNewIrradianceTexture);
	glDeleteTextures(1, &downsampledNewRadianceTexture);

	glDeleteRenderbuffers(1, &depth);

	glDeleteFramebuffers(1, &intermediateFramebuffer);
	glDeleteFramebuffers(1, &lightmapFramebuffer);

	//glReadBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//std::cout << "Function end: " << glfwGetTime() << std::endl;
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
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		doRadiosityIteration = false;
	}
}


//This function is adapted from https://learnopengl.com/In-Practice/Debugging
void Renderer::displayFramebufferTexture(ShaderLoader& debugShader, unsigned int& debugVAO, unsigned int textureID) {

	debugShader.useProgram();

	glActiveTexture(GL_TEXTURE0);
	debugShader.setUniformInt("framebufferTexture", 0);

	glBindTexture(GL_TEXTURE_2D, textureID);

	glBindVertexArray(debugVAO);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glUseProgram(0);
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