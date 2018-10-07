//A few lines for the window/context creation are from:
//https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/1.2.hello_window_clear/hello_window_clear.cpp 
//and
//http://www.opengl-tutorial.org/beginners-tutorials/tutorial-1-opening-a-window/

//There are also some Camera-related variables and functions here. I tried my best to point this out
//whenever there is a bigger chunk of Camera-related code. However, there might be instances where only a 
//line or two remain in this implementation. For those cases I would like to reiterate what I stated in Camera.cpp/Camera.h:
//The Camera related code is adapted from https://learnopengl.com/Getting-started/Camera
//(Note that this is not the case for the mouse_button_callback function, since that function handles lights, not Camera functionality)

#include "stdafx.h"

#include <iostream>
#include <vector>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assimp\Importer.hpp>

#include <Renderer\Renderer.h>
#include <Renderer\Camera.h>
#include <Renderer\ShaderLoader.h>

#include <Renderer\RadiosityConfig.h>

enum class RENDERER_RESOLUTION : int {
	RES_800x800 = 0,
	RES_720P = 1,
	RES_1080P = 2
};

enum class LIGHTMAP_RESOLUTION : int {
	RES_32 = 0,
	RES_64 = 1,
	RES_128 = 2,
	RES_256 = 3,
	RES_512 = 4
};

enum class ATTENUATION : int {
	ATT_LINEAR = 0,
	ATT_QUAD = 1,
	ATT_QUAD_AREA = 2
};

//This is the default value, properly set later
int ::RADIOSITY_TEXTURE_SIZE = 32;

unsigned int SCREEN_WIDTH = 1280;
unsigned int SCREEN_HEIGHT = 720;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void mouse_callback(GLFWwindow* window, double xPos, double yPos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);


//Camera-oriented global variables are based on https://learnopengl.com/Getting-started/Camera
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

//This is properly initialised in the main rendering function
float lampScale = 1.0;

Renderer::Renderer() {
	rendererResolution = 0;
	lightmapResolution = 0;
	attenuationType = 0;

	continuousUpdate = false;
	textureFiltering = false;
	multisampling = 0;
}

void Renderer::startRenderer(std::string objectFilepath) {
	if (!glfwInit()) {
		std::cout << "Failed to initialise GLFW" << std::endl;
		return;
	}

	if (rendererResolution == static_cast<int>(RENDERER_RESOLUTION::RES_800x800)) {
		SCREEN_WIDTH = 800;
		SCREEN_HEIGHT = 800;
	}
	else if (rendererResolution == static_cast<int>(RENDERER_RESOLUTION::RES_720P)) {
		SCREEN_WIDTH = 1280;
		SCREEN_HEIGHT = 720;
	}
	else if (rendererResolution == static_cast<int>(RENDERER_RESOLUTION::RES_1080P)) {
		SCREEN_WIDTH = 1920;
		SCREEN_HEIGHT = 1080;
	}

	if (lightmapResolution == static_cast<int>(LIGHTMAP_RESOLUTION::RES_32)) {
		::RADIOSITY_TEXTURE_SIZE = 32;
	}
	else if (lightmapResolution == static_cast<int>(LIGHTMAP_RESOLUTION::RES_64)) {
		::RADIOSITY_TEXTURE_SIZE = 64;
	}
	else if (lightmapResolution == static_cast<int>(LIGHTMAP_RESOLUTION::RES_128)) {
		::RADIOSITY_TEXTURE_SIZE = 128;
	}
	else if (lightmapResolution == static_cast<int>(LIGHTMAP_RESOLUTION::RES_256)) {
		::RADIOSITY_TEXTURE_SIZE = 256;
	}
	else if (lightmapResolution == static_cast<int>(LIGHTMAP_RESOLUTION::RES_512)) {
		::RADIOSITY_TEXTURE_SIZE = 512;
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

	//The resolve programs use the ShooterMeshSelection vertex shader, since both need to draw a screen-aligned quad

	ShaderLoader preprocessShader("../src/Shaders/Preprocess.vs", "../src/Shaders/Preprocess.fs");

	ShaderLoader preprocessShaderMultisample("../src/Shaders/PreprocessMultisample.vs", "../src/Shaders/PreprocessMultisample.fs");
	ShaderLoader preprocessResolveShader("../src/Shaders/ShooterMeshSelection.vs", "../src/Shaders/PreprocessResolve.fs");

	ShaderLoader shooterMeshSelectionShader("../src/Shaders/ShooterMeshSelection.vs", "../src/Shaders/ShooterMeshSelection.fs");

	ShaderLoader lightmapUpdateShader("../src/Shaders/LightmapUpdate.vs", "../src/Shaders/LightmapUpdate.fs");
	ShaderLoader lightmapUpdateShaderMultisample("../src/Shaders/LightmapUpdateMultisample.vs", "../src/Shaders/LightmapUpdateMultisample.fs");
	ShaderLoader lightmapResolveShader("../src/Shaders/ShooterMeshSelection.vs", "../src/Shaders/LightmapResolve.fs");

	ShaderLoader finalRenderShader("../src/Shaders/FinalRender.vs", "../src/Shaders/FinalRender.fs");

	ShaderLoader framebufferDebugShader("../src/Shaders/FramebufferDebug.vs", "../src/Shaders/FramebufferDebug.fs");

	ShaderLoader hemicubeVisibilityShader("../src/Shaders/HemicubeVisibilityTexture.vs", "../src/Shaders/HemicubeVisibilityTexture.fs");

	ObjectModel mainModel(objectFilepath, false, 1.0);
	ObjectModel lampModel("../assets/lamp.obj", true, 0.05);

	lampScale = lampModel.scale;

	int frameCounter = 0;
	double fpsTimeCounter = glfwGetTime();

	int iterationNumber = 0;

	//A quad that fills the whole screen
	float shooterMeshSelectionQuadVertices[] = {
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

	//Idea for these vertex coordinates is based on https://learnopengl.com/In-Practice/Debugging
	float quadVertices[] = {
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

		//This section is for the FPS counter and time-keeping for the camera
		//The camera-related parts are based on https://learnopengl.com/Getting-started/Camera 
		++frameCounter;
		
		float currentFrameTime = glfwGetTime();

		deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		//TODO: This could be made a separate function
		//Frame counter inspired by http://www.opengl-tutorial.org/miscellaneous/an-fps-counter/, additional features implemented here
		if ((currentFrameTime - fpsTimeCounter) >= 1.0) {
			double actualElapsedTime = (currentFrameTime - fpsTimeCounter);

			std::cout << "mSPF: " << ((actualElapsedTime * 1000) / (double)frameCounter) << " FPS: " << ((double)frameCounter / actualElapsedTime) << std::endl;

			frameCounter = 0;
			fpsTimeCounter += actualElapsedTime;
		}

		processInput(window);

		if (addLampMesh) {
			mainModel.addMesh(lampModel.meshes[0]);

			addLampMesh = !addLampMesh;
		}

		//Variables for drawing the scene
		glClearColor(1.0f, 0.898f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		finalRenderShader.useProgram();

		glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
		glm::mat4 ortho = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
		glm::mat4 view = camera.getViewMatrix();
		finalRenderShader.setUniformMat4("projection", projection);
		finalRenderShader.setUniformMat4("view", view);

		glm::mat4 model;
		model = glm::scale(model, glm::vec3(mainModel.scale));
		finalRenderShader.setUniformMat4("model", model);

		finalRenderShader.setUniformBool("addAmbient", addAmbient);

		glEnable(GL_CULL_FACE);

		//The preprocess function is called here
		if (preprocessDone == 1) {

			if (multisampling) {
				preprocessMultisample(mainModel, preprocessShaderMultisample, model, preprocessResolveShader, shooterMeshSelectionQuadVAO);
			}
			else {
				preprocess(mainModel, preprocessShader, model);
			}

			//This is needed so preprocess can only be done once
			std::cout << "Preprocess done" << std::endl;
			++preprocessDone;
		}

		//Radiosity variables
		glm::vec3 shooterRadiance;
		glm::vec3 shooterWorldspacePos;
		glm::vec3 shooterWorldspaceNormal;
		glm::vec2 shooterUV;
		int shooterMeshIndex;

		//Radiosity iteration section
		if (doRadiosityIteration) {

			if (debugInitialised) {
				for (unsigned int debugTexture : debugTextures) {
					glDeleteTextures(1, &debugTexture);
				}
			}

			if (meshSelectionNeeded) {
				shooterMesh = selectShooterMesh(mainModel, shooterMeshSelectionShader, shooterMeshSelectionQuadVAO);

				meshSelectionNeeded = false;
			}

			//Main body of the radiosity loop
			if (continuousUpdate) {
				selectMeshBasedShooter(mainModel, shooterRadiance, shooterWorldspacePos, shooterWorldspaceNormal, shooterUV, shooterMesh);

				std::cout << shooterWorldspaceNormal.x << std::endl;
				std::cout << shooterWorldspaceNormal.y << std::endl;
				std::cout << shooterWorldspaceNormal.z << std::endl;

				std::vector<glm::mat4> shooterViews;

				unsigned int visibilityTextureSize = 1024;

				glm::mat4 shooterProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

				hemicubeVisibilityShader.useProgram();
				hemicubeVisibilityShader.setUniformMat4("projection", shooterProj);

				std::vector<unsigned int> visibilityTextures = createHemicubeTextures(mainModel, hemicubeVisibilityShader, model, shooterViews, visibilityTextureSize, shooterWorldspacePos, shooterWorldspaceNormal);

				debugTextures = visibilityTextures;

				debugTexture = visibilityTextures[1];
				debugInitialised = true;

				if (multisampling) {
					lightmapUpdateShaderMultisample.useProgram();
					lightmapUpdateShaderMultisample.setUniformMat4("projection", shooterProj);
					lightmapUpdateShaderMultisample.setUniformVec3("shooterRadiance", shooterRadiance);
					lightmapUpdateShaderMultisample.setUniformVec3("shooterWorldspacePos", shooterWorldspacePos);
					lightmapUpdateShaderMultisample.setUniformVec3("shooterWorldspaceNormal", shooterWorldspaceNormal);
					lightmapUpdateShaderMultisample.setUniformVec2("shooterUV", shooterUV);
					updateLightmapsMultisample(mainModel, lightmapUpdateShaderMultisample, model, shooterViews, visibilityTextures, lightmapResolveShader, shooterMeshSelectionQuadVAO);
				}
				else {
					lightmapUpdateShader.useProgram();
					lightmapUpdateShader.setUniformMat4("projection", shooterProj);
					lightmapUpdateShader.setUniformVec3("shooterRadiance", shooterRadiance);
					lightmapUpdateShader.setUniformVec3("shooterWorldspacePos", shooterWorldspacePos);
					lightmapUpdateShader.setUniformVec3("shooterWorldspaceNormal", shooterWorldspaceNormal);
					lightmapUpdateShader.setUniformVec2("shooterUV", shooterUV);
					updateLightmaps(mainModel, lightmapUpdateShader, model, shooterViews, visibilityTextures);
				}

				for (unsigned int debugTexture : debugTextures) {
					glDeleteTextures(1, &debugTexture);
				}

				std::cout << iterationNumber << std::endl;

				++iterationNumber;

				//Uncomment this to resume manual iteration
				//doRadiosityIteration = false;
			}
			else {
				while (!meshSelectionNeeded) {
					selectMeshBasedShooter(mainModel, shooterRadiance, shooterWorldspacePos, shooterWorldspaceNormal, shooterUV, shooterMesh);

					std::cout << shooterWorldspaceNormal.x << std::endl;
					std::cout << shooterWorldspaceNormal.y << std::endl;
					std::cout << shooterWorldspaceNormal.z << std::endl;

					//Used as an "out" variable for createHemicubeTextures()
					std::vector<glm::mat4> shooterViews;

					unsigned int visibilityTextureSize = 1024;

					glm::mat4 shooterProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

					hemicubeVisibilityShader.useProgram();
					hemicubeVisibilityShader.setUniformMat4("projection", shooterProj);

					std::vector<unsigned int> visibilityTextures = createHemicubeTextures(mainModel, hemicubeVisibilityShader, model, shooterViews, visibilityTextureSize, shooterWorldspacePos, shooterWorldspaceNormal);

					debugTextures = visibilityTextures;

					debugTexture = visibilityTextures[1];
					debugInitialised = true;

					if (multisampling) {
						lightmapUpdateShaderMultisample.useProgram();
						lightmapUpdateShaderMultisample.setUniformMat4("projection", shooterProj);
						lightmapUpdateShaderMultisample.setUniformVec3("shooterRadiance", shooterRadiance);
						lightmapUpdateShaderMultisample.setUniformVec3("shooterWorldspacePos", shooterWorldspacePos);
						lightmapUpdateShaderMultisample.setUniformVec3("shooterWorldspaceNormal", shooterWorldspaceNormal);
						lightmapUpdateShaderMultisample.setUniformVec2("shooterUV", shooterUV);
						updateLightmapsMultisample(mainModel, lightmapUpdateShaderMultisample, model, shooterViews, visibilityTextures, lightmapResolveShader, shooterMeshSelectionQuadVAO);
					}
					else {
						lightmapUpdateShader.useProgram();
						lightmapUpdateShader.setUniformMat4("projection", shooterProj);
						lightmapUpdateShader.setUniformVec3("shooterRadiance", shooterRadiance);
						lightmapUpdateShader.setUniformVec3("shooterWorldspacePos", shooterWorldspacePos);
						lightmapUpdateShader.setUniformVec3("shooterWorldspaceNormal", shooterWorldspaceNormal);
						lightmapUpdateShader.setUniformVec2("shooterUV", shooterUV);
						updateLightmaps(mainModel, lightmapUpdateShader, model, shooterViews, visibilityTextures);
					}

					for (unsigned int debugTexture : debugTextures) {
						glDeleteTextures(1, &debugTexture);
					}

					std::cout << iterationNumber << std::endl;

					++iterationNumber;

					}
					//This line makes it sure that the scene is observable after the current shooter mesh is done shooting
					doRadiosityIteration = false;
			}
		}

		//Render the scene with the lightmaps calculated up until this point
		int lampCounter = 0;

		for (unsigned int i = 0; i < mainModel.meshes.size(); ++i) {
			if (mainModel.meshes[i].isLamp) {
				finalRenderShader.useProgram();

				finalRenderShader.setUniformMat4("projection", projection);
				finalRenderShader.setUniformMat4("view", view);

				glm::mat4 lampModel = glm::mat4();

				lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
				lampModel = glm::scale(lampModel, glm::vec3(lampScale));
				//The uniform for the lamp's model is just "model"
				finalRenderShader.setUniformMat4("model", lampModel);

				
				unsigned int irradianceID;
				glGenTextures(1, &irradianceID);

				glBindTexture(GL_TEXTURE_2D, irradianceID);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, &mainModel.meshes[i].irradianceData[0]);

				//There is no point trying to filter a lamp since it is equally bright everywhere
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

				//If the user desires texture filtering use linear filtering
				if (textureFiltering) {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}
				else {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				}


				//This prevents some lightmap bleeding if texture filtering is on, otherwise does nothing significant
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glActiveTexture(GL_TEXTURE0);
				finalRenderShader.setUniformInt("irradianceTexture", 0);
				glBindTexture(GL_TEXTURE_2D, irradianceID);
				

				mainModel.meshes[i].draw(finalRenderShader);

				glDeleteTextures(1, &irradianceID);
			}
		}

		//If framebuffer deugging is needed, check if debugInitialised == true
		if (false) {
			displayFramebufferTexture(framebufferDebugShader, quadVAO, debugTexture);
		}
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
}

//The functions below (until the processInput function) handle the radiosity functionality

//Most of the OpenGL resources (textures and framebuffers) could be created as what would be the equivalent of object variables, however, 
//in the current implementation they are created locally - I feel it makes the code slightly more understandable (since the resources are
//declared and created locally) but might introduce a slight performance overhead - when optimising for production transitioning to the
//former way would likely be beneficial

//This function has to be called by the user after the lights are placed but before the start of the radiosity iteration
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

		//Create readback buffers for all of the texture data
		std::vector<GLfloat> worldspacePositionDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
		std::vector<GLfloat> normalVectorDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);

		std::vector<GLfloat> idDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
		std::vector<GLfloat> uvDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);

		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(lampScale));

			shader.setUniformMat4("model", lampModel);

			model.meshes[i].draw(shader);

			++lampCounter;
		}
		else {
			shader.setUniformMat4("model", mainObjectModelMatrix);

			model.meshes[i].draw(shader);
		}

		//Read the data back to the CPU
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

		//Find the texels that have triangles mapped to them
		for (unsigned int j = 0; j < model.meshes[i].idData.size(); j += 3) {
			float redIDValue = model.meshes[i].idData[j];
			float greenIDValue = model.meshes[i].idData[j + 1];
			float blueIDValue = model.meshes[i].idData[j + 2];

			float idSum = redIDValue + greenIDValue + blueIDValue;

			if (idSum > 0) {
				model.meshes[i].texturespaceShooterIndices.push_back(j);
			}
		}

		if (attenuationType == static_cast<int>(ATTENUATION::ATT_LINEAR) || attenuationType == static_cast<int>(ATTENUATION::ATT_QUAD)) {
			model.meshes[i].texelArea = 1.0;
		}
		else {
			//1.0 + ... is to prevent downscaling, which usually makes scenes way too dark
			model.meshes[i].texelArea = 1.0 + model.meshes[i].overallArea / model.meshes[i].texturespaceShooterIndices.size();
		}
	}

	//Delete textures here if they are not needed anymore
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//Only to be called after the preprocess function
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

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		unsigned int meshID = i;


		//Calculate an RGB ID for the mesh based on the mesh ID
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

		//Draw a full, screen-aligned quad
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDeleteTextures(1, &mipmappedRadianceTexture);
	}

	//Read back the chosen mesh ID
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

	glDeleteFramebuffers(1, &shooterSelectionFramebuffer);
	glDeleteRenderbuffers(1, &depth);
	glDeleteTextures(1, &meshIDTexture);

	return chosenShooterMeshID;
}

void Renderer::selectMeshBasedShooter(ObjectModel& model, glm::vec3& shooterRadiance, glm::vec3& shooterWorldspacePos, glm::vec3& shooterWorldspaceNormal, glm::vec2& shooterUV, unsigned int& shooterMeshIndex) {
	unsigned int texelIndex = model.meshes[shooterMeshIndex].texturespaceShooterIndices[shooterIndex];

	//Find the radiance of the next shooter
	if (attenuationType == static_cast<int>(ATTENUATION::ATT_LINEAR) || attenuationType == static_cast<int>(ATTENUATION::ATT_QUAD)) {
		shooterRadiance = glm::vec3(
			model.meshes[shooterMeshIndex].radianceData[texelIndex] / model.meshes[shooterMeshIndex].texturespaceShooterIndices.size(),
			model.meshes[shooterMeshIndex].radianceData[texelIndex + 1] / model.meshes[shooterMeshIndex].texturespaceShooterIndices.size(),
			model.meshes[shooterMeshIndex].radianceData[texelIndex + 2] / model.meshes[shooterMeshIndex].texturespaceShooterIndices.size()
		);
	}
	else {
		shooterRadiance = glm::vec3(
			model.meshes[shooterMeshIndex].radianceData[texelIndex] / model.meshes[shooterMeshIndex].texturespaceShooterIndices.size() * model.meshes[shooterMeshIndex].texelArea,
			model.meshes[shooterMeshIndex].radianceData[texelIndex + 1] / model.meshes[shooterMeshIndex].texturespaceShooterIndices.size() * model.meshes[shooterMeshIndex].texelArea,
			model.meshes[shooterMeshIndex].radianceData[texelIndex + 2] / model.meshes[shooterMeshIndex].texturespaceShooterIndices.size() * model.meshes[shooterMeshIndex].texelArea
		);
	}

	//Find the other necessary information of the shooter
	shooterWorldspacePos = glm::vec3(model.meshes[shooterMeshIndex].worldspacePosData[texelIndex], model.meshes[shooterMeshIndex].worldspacePosData[texelIndex + 1], model.meshes[shooterMeshIndex].worldspacePosData[texelIndex + 2]);

	shooterWorldspaceNormal = glm::vec3(model.meshes[shooterMeshIndex].worldspaceNormalData[texelIndex], model.meshes[shooterMeshIndex].worldspaceNormalData[texelIndex + 1], model.meshes[shooterMeshIndex].worldspaceNormalData[texelIndex + 2]);

	shooterUV = glm::vec3(model.meshes[shooterMeshIndex].uvData[texelIndex], model.meshes[shooterMeshIndex].uvData[texelIndex + 1], model.meshes[shooterMeshIndex].uvData[texelIndex + 2]);

	//Set the shooter's RGB radiance to 0
	model.meshes[shooterMeshIndex].radianceData[texelIndex] = 0.0f;
	model.meshes[shooterMeshIndex].radianceData[texelIndex + 1] = 0.0f;
	model.meshes[shooterMeshIndex].radianceData[texelIndex + 2] = 0.0f;


	//Check if mesh selection is needed, if not then iterate
	if (shooterIndex >= model.meshes[shooterMeshIndex].texturespaceShooterIndices.size() - 1) {
		meshSelectionNeeded = true;

		shooterIndex = 0;
	}
	else {
		++shooterIndex;
	}
}


//Old shooter selection function: selects the texel with the highest energy but it is incredibly slow - replaced by the mesh-based shooter selection functions
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

//Old visibility texture function - creates a texture with hemisphere visibility calculation - produced way too many artifacts and thus it is replaced by the hemicube visibility function
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
			lampModel = glm::scale(lampModel, glm::vec3(lampScale));

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
std::vector<unsigned int> Renderer::createHemicubeTextures(
	ObjectModel& model,
	ShaderLoader& hemicubeShader,
	glm::mat4& mainObjectModelMatrix,
	std::vector<glm::mat4>& viewMatrices,
	unsigned int& resolution,
	glm::vec3& shooterWorldspacePos,
	glm::vec3& shooterWorldspaceNormal) {

	float borderColour[] = { 0.0f, 0.0f, 0.0f, 1.0f };


	//For an explanation take a look at the "double up" part in the report
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

	glm::mat4 frontShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + shooterWorldspaceNormal, hemicubeUp);

	glm::mat4 leftShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + (-hemicubeRight), hemicubeUp);
	glm::mat4 rightShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + hemicubeRight, hemicubeUp);

	glm::mat4 upShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + hemicubeUp, -normalisedShooterNormal);
	glm::mat4 downShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + (-hemicubeUp), normalisedShooterNormal);

	viewMatrices.push_back(frontShooterView);

	viewMatrices.push_back(leftShooterView);
	viewMatrices.push_back(rightShooterView);

	viewMatrices.push_back(upShooterView);
	viewMatrices.push_back(downShooterView);


	unsigned int hemicubeFramebuffer;
	glGenFramebuffers(1, &hemicubeFramebuffer);

	//The upcoming part renders the scene 5 times (for each hemicube face)
	//Each render pass could be abstracted away into a function in order to eliminate code repetition, however,
	//I find that this way the code is slightly easier to understand, since everything is in one place

	/*Alternate solution:
	First let's have a function that creates a texture for a hemicube depth map

	unsigned int Renderer::createHemicubeDepthMapTexture(const unsigned int resolution) {
		float borderColour[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		unsigned int depthMapTexture;

		glGenTextures(1, &depthMapTexture);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		//This part is needed to avoid light bleeding by oversampling (so sampling outside the depth texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

		return depthMapTexture;
	}


	After, in the function we are at now:

	unsigned int frontDepthMap = createHemicubeDepthMapTexture(resolution);
	unsigned int leftDepthMap = createHemicubeDepthMapTexture(resolution);
	unsigned int rightDepthMap = createHemicubeDepthMapTexture(resolution);
	unsigned int upDepthMap = createHemicubeDepthMapTexture(resolution);
	unsigned int downDepthMap = createHemicubeDepthMapTexture(resolution);

	std::vector<unsigned int> depthTextures;

	depthTextures.push_back(frontDepthMap);

	depthTextures.push_back(leftDepthMap);
	depthTextures.push_back(rightDepthMap);

	depthTextures.push_back(upDepthMap);
	depthTextures.push_back(downDepthMap);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFramebuffer);

	//Loop 5 times, for each of the hemicube faces
	for (unsigned int i = 0; i < 5; ++i) {

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextures[i], 0);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		glViewport(0, 0, resolution, resolution);

		//Need to get rid of this when/if we cut off half of the depth maps
		glClear(GL_DEPTH_BUFFER_BIT);

		int lampCounter = 0;

		for (unsigned int i = 0; i < model.meshes.size(); ++i) {

			hemicubeShader.useProgram();

			hemicubeShader.setUniformMat4("view", viewMatrices[i]);


			if (model.meshes[i].isLamp) {
				glm::mat4 lampModel = glm::mat4();

				lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
				lampModel = glm::scale(lampModel, glm::vec3(lampScale));

				hemicubeShader.setUniformMat4("model", lampModel);

				model.meshes[i].draw(hemicubeShader);

				++lampCounter;
			}
			else {
				hemicubeShader.setUniformMat4("model", mainObjectModelMatrix);

				model.meshes[i].draw(hemicubeShader);
			}
		}
	}


	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteFramebuffers(1, &hemicubeFramebuffer);

	return depthTextures;


	Instead of the code below, this avoids a bunch of repetition but might be a bit harder to understand so I'll leave both
	*/

	unsigned int frontDepthMap;
	glGenTextures(1, &frontDepthMap);
	glBindTexture(GL_TEXTURE_2D, frontDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//This part is needed to avoid light bleeding by oversampling (so sampling outside the depth texture)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, frontDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glViewport(0, 0, resolution, resolution);

	//Need to get rid of this when/if we cut off half of the depth maps
	glClear(GL_DEPTH_BUFFER_BIT);

	int lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		hemicubeShader.useProgram();

		hemicubeShader.setUniformMat4("view", frontShooterView);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(lampScale));

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
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, leftDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glViewport(0, 0, resolution, resolution);

	glClear(GL_DEPTH_BUFFER_BIT);

	lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		hemicubeShader.useProgram();

		hemicubeShader.setUniformMat4("view", leftShooterView);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(lampScale));

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
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rightDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glViewport(0, 0, resolution, resolution);

	glClear(GL_DEPTH_BUFFER_BIT);

	lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		hemicubeShader.useProgram();

		hemicubeShader.setUniformMat4("view", rightShooterView);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(lampScale));

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
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, upDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glViewport(0, 0, resolution, resolution);

	glClear(GL_DEPTH_BUFFER_BIT);

	lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		hemicubeShader.useProgram();

		hemicubeShader.setUniformMat4("view", upShooterView);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(lampScale));

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
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

	glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, downDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glViewport(0, 0, resolution, resolution);

	glClear(GL_DEPTH_BUFFER_BIT);

	lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		hemicubeShader.useProgram();

		hemicubeShader.setUniformMat4("view", downShooterView);


		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(lampScale));

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

	glDeleteFramebuffers(1, &hemicubeFramebuffer);


	return depthTextures;
}

//The shooter information has to be bound to the lightmapUpdateShader before calling this function
void Renderer::updateLightmaps(ObjectModel& model, ShaderLoader& lightmapUpdateShader, glm::mat4& mainObjectModelMatrix, std::vector<glm::mat4>& viewMatrices, std::vector<unsigned int>& visibilityTextures) {

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

		lightmapUpdateShader.useProgram();

		lightmapUpdateShader.setUniformMat4("view", viewMatrices[0]);

		lightmapUpdateShader.setUniformMat4("leftView", viewMatrices[1]);
		lightmapUpdateShader.setUniformMat4("rightView", viewMatrices[2]);

		lightmapUpdateShader.setUniformMat4("upView", viewMatrices[3]);
		lightmapUpdateShader.setUniformMat4("downView", viewMatrices[4]);

		lightmapUpdateShader.setUniformFloat("texelArea", model.meshes[i].texelArea);
		lightmapUpdateShader.setUniformInt("attenuationType", attenuationType);


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
		
		//Also bind the visibility textures
		glActiveTexture(GL_TEXTURE2);
		lightmapUpdateShader.setUniformInt("visibilityTexture", 2);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[0]);

		glActiveTexture(GL_TEXTURE3);
		lightmapUpdateShader.setUniformInt("leftVisibilityTexture", 3);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[1]);

		glActiveTexture(GL_TEXTURE4);
		lightmapUpdateShader.setUniformInt("rightVisibilityTexture", 4);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[2]);

		glActiveTexture(GL_TEXTURE5);
		lightmapUpdateShader.setUniformInt("upVisibilityTexture", 5);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[3]);

		glActiveTexture(GL_TEXTURE6);
		lightmapUpdateShader.setUniformInt("downVisibilityTexture", 6);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[4]);

		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(lampScale));

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

		//Read back the new ir/radiance values
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &newIrradianceDataBuffer[0]);
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &newRadianceDataBuffer[0]);

		model.meshes[i].irradianceData = newIrradianceDataBuffer;
		model.meshes[i].radianceData = newRadianceDataBuffer;

		glDeleteTextures(1, &irradianceID);
		glDeleteTextures(1, &radianceID);
	}

	glDeleteTextures(1, &newIrradianceTexture);
	glDeleteTextures(1, &newRadianceTexture);
	glDeleteRenderbuffers(1, &depth);
	glDeleteFramebuffers(1, &lightmapFramebuffer);

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//Multisampled version of the preprocess function
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
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, worldspacePosData, 0);

	//Create texture to hold worldspace-normal data
	glGenTextures(1, &worldspaceNormalData);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, worldspaceNormalData);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, worldspaceNormalData, 0);

	//Create texture to hold triangle IDs in texturespace
	glGenTextures(1, &idData);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, idData);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D_MULTISAMPLE, idData, 0);

	//Create texture to hold UV-coordinate data in texturespace
	glGenTextures(1, &uvData);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, uvData);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_TRUE);
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
			lampModel = glm::scale(lampModel, glm::vec3(lampScale));

			shader.setUniformMat4("model", lampModel);

			model.meshes[i].draw(shader);

			++lampCounter;
		}
		else {
			shader.setUniformMat4("model", mainObjectModelMatrix);

			model.meshes[i].draw(shader);
		}

		//Do a custom resolve pass
		glBindFramebuffer(GL_FRAMEBUFFER, intermediateFramebuffer);
		glClear(GL_COLOR_BUFFER_BIT);

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

		//Read back the resolved values
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


		//Find the texels that have trianlges mapped to them
		for (unsigned int j = 0; j < model.meshes[i].idData.size(); j += 3) {
			float redIDValue = model.meshes[i].idData[j];
			float greenIDValue = model.meshes[i].idData[j + 1];
			float blueIDValue = model.meshes[i].idData[j + 2];

			float idSum = redIDValue + greenIDValue + blueIDValue;

			if (idSum > 0) {
				model.meshes[i].texturespaceShooterIndices.push_back(j);
			}
		}


		if (attenuationType == static_cast<int>(ATTENUATION::ATT_LINEAR) || attenuationType == static_cast<int>(ATTENUATION::ATT_QUAD)) {
			model.meshes[i].texelArea = 1.0;
		}
		else {
			//1.0 + ... is to prevent downscaling, which usually makes scenes way too dark
			model.meshes[i].texelArea = 1.0 + model.meshes[i].overallArea / model.meshes[i].texturespaceShooterIndices.size();
		}
	}
	//Delete textures here if they are not needed anymore
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//Multisampled version of the lightmap update function
void Renderer::updateLightmapsMultisample(ObjectModel& model, ShaderLoader& lightmapUpdateShader, glm::mat4& mainObjectModelMatrix, std::vector<glm::mat4>& viewMatrices, std::vector<unsigned int>& visibilityTextures, ShaderLoader& resolveShader, unsigned int& screenAlignedQuadVAO) {

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
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, newIrradianceTexture, 0);

	glGenTextures(1, &newRadianceTexture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, newRadianceTexture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB32F, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_TRUE);
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

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, lightmapFramebuffer);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightmapUpdateShader.useProgram();

		std::vector<GLfloat> newIrradianceDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
		std::vector<GLfloat> newRadianceDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);

		lightmapUpdateShader.useProgram();

		lightmapUpdateShader.setUniformMat4("view", viewMatrices[0]);

		lightmapUpdateShader.setUniformMat4("leftView", viewMatrices[1]);
		lightmapUpdateShader.setUniformMat4("rightView", viewMatrices[2]);

		lightmapUpdateShader.setUniformMat4("upView", viewMatrices[3]);
		lightmapUpdateShader.setUniformMat4("downView", viewMatrices[4]);

		lightmapUpdateShader.setUniformFloat("texelArea", model.meshes[i].texelArea);
		lightmapUpdateShader.setUniformInt("attenuationType", attenuationType);

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

		//Also bind the visibility textures
		glActiveTexture(GL_TEXTURE2);
		lightmapUpdateShader.setUniformInt("visibilityTexture", 2);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[0]);

		glActiveTexture(GL_TEXTURE3);
		lightmapUpdateShader.setUniformInt("leftVisibilityTexture", 3);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[1]);

		glActiveTexture(GL_TEXTURE4);
		lightmapUpdateShader.setUniformInt("rightVisibilityTexture", 4);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[2]);

		glActiveTexture(GL_TEXTURE5);
		lightmapUpdateShader.setUniformInt("upVisibilityTexture", 5);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[3]);

		glActiveTexture(GL_TEXTURE6);
		lightmapUpdateShader.setUniformInt("downVisibilityTexture", 6);
		glBindTexture(GL_TEXTURE_2D, visibilityTextures[4]);

		if (model.meshes[i].isLamp) {
			glm::mat4 lampModel = glm::mat4();

			lampModel = glm::translate(lampModel, lightLocations[lampCounter]);
			lampModel = glm::scale(lampModel, glm::vec3(lampScale));

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


		//Do the custom resolve pass
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

		//Read back the resolved lightmap values
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &newIrradianceDataBuffer[0]);
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &newRadianceDataBuffer[0]);

		model.meshes[i].irradianceData = newIrradianceDataBuffer;
		model.meshes[i].radianceData = newRadianceDataBuffer;

		glDeleteTextures(1, &irradianceID);
		glDeleteTextures(1, &radianceID);
	}

	glDeleteTextures(1, &newIrradianceTexture);
	glDeleteTextures(1, &newRadianceTexture);

	glDeleteTextures(1, &downsampledNewIrradianceTexture);
	glDeleteTextures(1, &downsampledNewRadianceTexture);

	glDeleteRenderbuffers(1, &depth);

	glDeleteFramebuffers(1, &intermediateFramebuffer);
	glDeleteFramebuffers(1, &lightmapFramebuffer);

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	//The WASD keys are Camera-related and thus are based on
	//https://learnopengl.com/Getting-started/Camera
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
//Right now it is not in use
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

void Renderer::setParameters(int& rendererResolution, int& lightmapResolution, int& attenuationType, bool& continuousUpdate, bool& textureFiltering, bool& multisampling) {
	this->rendererResolution = rendererResolution;
	this->lightmapResolution = lightmapResolution;
	this->attenuationType = attenuationType;

	this->continuousUpdate = continuousUpdate;
	this->textureFiltering = textureFiltering;
	this->multisampling = multisampling;
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


//This function is related to the Camera and thus it is based on
//https://learnopengl.com/Getting-started/Camera
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