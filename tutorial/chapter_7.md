# Chapter 7 - Putting it together

...and also start thinking about integrating a frontend. Let's start with the latter. The frontend is going to support a few features. In no particular order:

* Launchng the renderer
* Loading a scene file
* Having a settings menu where the user can change:
    * The resolution of the renderer
    * The resolution of the lightmaps
    * The attenuation type
    * Whether to continuously update the lightmaps or not
    * Whether to texture filter the lightmaps (covered in the next tutorial)
    * Whether to multisample the lightmaps (covered in the next tutorial)

On the renderer side the settings that have multiple possible values (instead of just true/false) are going to be handled with enums. If you haven't used them before, they are pretty simple - they're basically integers that are given a special name, so you don't have to use `if (resolution == 2) {}` but can use `if (resolution == static_cast<int>(RENDERER_RESOLUTION::RES_800x800))`. The frontend passes an `int` (instead of the defined enum) to the renderer so that is why the static cast is needed.

The frontend is going to be presented in detail in an after-release bonus chapter.

This tutorial is mostly going to be about calling the functions discussed in the previous tutorials but they require some other things too so I'll include the OpenGL initialisation steps as well.

Before we start anything, the radiosity texture size is used in multiple different files/classes so in order to not have the compiler complain about that it is declared in its own header file, as an `extern`:

RadiosityConfig.h
```cpp
#ifndef RADIOSITYCONFIG_H
#define RADIOSITYCONFIG_H

extern int RADIOSITY_TEXTURE_SIZE;

#endif
```

Afterwards, in our Renderer.cpp file let's define our enums: 

```cpp
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
```

Set some of the default values:

```cpp
int ::RADIOSITY_TEXTURE_SIZE = 32;

unsigned int SCREEN_WIDTH = 1280;
unsigned int SCREEN_HEIGHT = 720;
```

Set the variables related to the camera and the frame counter:

```cpp
//Camera-oriented global variables are based on https://learnopengl.com/Getting-started/Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCREEN_WIDTH / 2.0f;
float lastY = SCREEN_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrameTime = 0.0f;
```

The variables related to lamp placement:

```cpp
std::vector<glm::vec3> lightLocations = std::vector<glm::vec3>();
bool addLampMesh = false;
bool addAmbient = false;
```

Here, `addLampMesh` is worth a few words. The way lamps are added to the scene is by inserting a new Mesh into the scene model. This is done at the next iteration, if the `addLampMesh` variable is true. Once a lamp is added it is set to false.

Finally, some of the radiosity variables are set here, along with the scale of our lamps:

```cpp
bool doRadiosityIteration = false;

unsigned int preprocessDone = 0;

bool meshSelectionNeeded = true;
unsigned int shooterIndex = 0;
unsigned int shooterMesh = 0;

//This is properly initialised in the main rendering function
float lampScale = 1.0;
```

Now let's look at the constructor of the `Renderer` class. In the frontend you'll see that I decided to create a `Renderer` object when the program starts. This means that we don't have access to the user-defined configuration values at that time so the constructor only sets a few default values. We, however, do have access to the configuration values when the user starts the renderer. To set these values a small helper function is used in the event handler for the start button: `void setParameters(int& rendererResolution, int& lightmapResolution, int& attenuationType, bool& continuousUpdate, bool& textureFiltering, bool& multisampling);`. I'll talk more about this in the frontend chapter. An alternate solution is to only construct the renderer object in the start button event handler and modify the `Renderer` constructor so you can directly pass those values to it. Choose the one you prefer, I'll show the former in this tutorial series:

```cpp
Renderer::Renderer() {
	rendererResolution = 0;
	lightmapResolution = 0;
	attenuationType = 0;

	continuousUpdate = false;
	textureFiltering = false;
	multisampling = 0;
}
```

The next code block sets some of the config variables and initialises OpenGL:

```cpp
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
```

After this, we'll load the shaders we're going to use, which means creating a bunch of `ShaderLoader` objects. For now, you're not supposed to know about the multisample and the resolve shaders. We'll cover those in their own chapter but the rest should be familiar from the previous chapters.

```cpp
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
```

Next, let's load the main object model, the lamp model, set the scale of the lamp model and initialise a few counters:

```cpp
	ObjectModel mainModel(objectFilepath, false, 1.0);
	ObjectModel lampModel("../assets/lamp.obj", true, 0.05);

	lampScale = lampModel.scale;

	int frameCounter = 0;
	double fpsTimeCounter = glfwGetTime();

	int iterationNumber = 0;
```

After this, we are going to need the vertices and VAO/VBO for the full screen quad that is used for the shooter mesh selection step:

```cpp
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
```

Finally, for the initialisation stages we declare some debug variables. They are not in use right now but with these it is possible to print a normally off-screen texture to a part of the screen:

```cpp
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
```

All right, time to start the render loop. First, the frame time counter functionality:

```cpp
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
```

Afterwards let's handle the user input. This is done in the `processInput()` function. It is rather self-explanatory so take a look! It is a function of the `Renderer` class so declare it outside the render loop:

```cpp
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
```

Let's get back to the render loop. After the frame time counter we'll handle the user input and add a lamp mesh to our main scene model if the user placed a lamp:

```cpp
		processInput(window);

		if (addLampMesh) {
			mainModel.addMesh(lampModel.meshes[0]);

			addLampMesh = !addLampMesh;
		}
```

To get them out of the way, let's declare the variables (mostly transformation matrices, really) we are going to need when we draw the actual scene to the screen. I've not mentioned the `addAmbient` variable yet. As the name implies it is used for adding a small amount of ambient light to the scene. This is useful for navigating the scene before the radiosity calculation. This can be switched on/off at will by the user:

```cpp
		//Variables for drawing the scene
		glClearColor(1.0f, 0.898f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		finalRenderShader.useProgram();

		glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.getViewMatrix();
		finalRenderShader.setUniformMat4("projection", projection);
		finalRenderShader.setUniformMat4("view", view);

		glm::mat4 model;
		model = glm::scale(model, glm::vec3(mainModel.scale));
		finalRenderShader.setUniformMat4("model", model);

		finalRenderShader.setUniformBool("addAmbient", addAmbient);

		glEnable(GL_CULL_FACE);
```

After this comes the first step in the radiosity calculation: the preprocessing. This has both a normal and a multisampled variant. Since the preprocess step is user-initiated and should be only used once we use the `preprocessDone` as a 3-way variable. If it is 0, it means the preprocess step should not be done during this iteration but the user is allowed to start it. If it is 1 it means the preprocess step should be done. If it is 2 the preprocess step has been done so the user is not allowed to start it again. It is possible to change this to an enum so if you prefer that, don't let me stop you! Also note that we use the shooterMeshSelectionQuadVAO since for the multisampling resolve step we need a screen-aligned quad (just like the shooter mesh selection, just for a different purpose). Again, more about that in the next tutorial.

```cpp
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
```

Let's declare the radiosity variables:

```cpp
		//Radiosity variables
		glm::vec3 shooterRadiance;
		glm::vec3 shooterWorldspacePos;
		glm::vec3 shooterWorldspaceNormal;
		glm::vec2 shooterUV;
		int shooterMeshIndex;
```

Then start the radiosity iteration. I also left some debug code there but it is not properly activated. What it did was grabbing all of the visibility textures produced by the hemicube algorithm and drew it to the screen. This can be switched on by modifying one variable near the end of the render loop, I'll mention it when we get there.

```cpp
		//Radiosity iteration section
		if (doRadiosityIteration) {

			if (debugInitialised) {
				for (unsigned int debugTexture : debugTextures) {
					glDeleteTextures(1, &debugTexture);
				}
			}
```

Before we get to the sub-iteration loop (i.e. to the loop where one shooter is processed per iteration) we have to do the shooter mesh selection:

```cpp
			if (meshSelectionNeeded) {
				shooterMesh = selectShooterMesh(mainModel, shooterMeshSelectionShader, shooterMeshSelectionQuadVAO);

				meshSelectionNeeded = false;
			}
```

After this, we can get to the main radiosity loop. There are two ways this loop can run - either continuously, in which case the scene gets updated dynamically or "off-screen", in which case a mesh gets processed then the scene gets updated. The only difference between the two is that the latter runs in a `while(!meshSelectionNeeded)` loop so I'll only present the code for the continuous update here, that is the mode I usually use:

```cpp
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
```

If you've been following the tutorial series this block should be nothing surprising. We have to select the shooter, declare a few variables for the visibility textures, create the visibility textures, set a few uniforms for the lightmap update and finally, update the lightmaps. As I said previously, the multisampled part is going to be covered in the next chapter so just roll with it for now.

That concludes the radiosity iteration! We have one more thing to do, however, which is rendering the scene itself with our lightmaps. Let's get to it. As you've seen previously, the lamps and the general geometry are rendered separately. Otherwise, the only interesting thing that has to be done is to create textures out of the irradiance vectors (we don't need the radiance values since the light that is visible to the observer is the reflected, i.e. irradiant light). We also have to delete these once we are done with them. First, let's look at the vertex and fragment shaders:

FinalRender.vs
```glsl
#version 450 core

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTextureCoord;
layout (location = 3) in vec3 inID;

out vec3 fragPos;
out vec3 normal;
out vec2 textureCoord;
out vec3 ID;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    fragPos = vec3(model * vec4(vertexPos, 1.0));

    normal = mat3(transpose(inverse(model))) * inNormal;

    textureCoord = inTextureCoord;

    ID = inID;

    gl_Position = projection * view * vec4(fragPos, 1.0);
}
```

As you can see, it is incredibly simple. It doesn't do more than transforming the scene to clip space (the viewpoint being the camera of the user). It also passes the triangle ID to the fragment shader but it is unused.

FinalRender.fs
```glsl
#version 450 core

out vec4 fragColour;

in vec2 textureCoord;

in vec3 ID;

uniform sampler2D irradianceTexture;
//uniform sampler2D radianceTexture;

uniform sampler2D texture_diffuse0;

uniform int addAmbient;

void main() {
    vec3 result = vec3(0.0f, 0.0f, 0.0f);

    vec3 ambient = vec3(0.2, 0.2, 0.2) * texture(texture_diffuse0, textureCoord).rgb;

    if (addAmbient == 1) {
        result += ambient;
    }

    //Retrieve the irradiance value from the irradiance texture
    vec3 irradianceValue = texture(irradianceTexture, textureCoord).rgb;

    vec3 diffuseValue = texture(texture_diffuse0, textureCoord).rgb;

    result += irradianceValue * diffuseValue;

    //Clamp the result to the diffuse value    
    if (result.r > diffuseValue.r) {
        result.r = diffuseValue.r;
    }
    if (result.g > diffuseValue.g) {
        result.g = diffuseValue.g;
    }
    if (result.b > diffuseValue.b) {
        result.b = diffuseValue.b;
    }
    
    //Apply gamma correction
    result = pow(result, vec3(1.0/2.2));

    fragColour = vec4(result, 1.0);
}
```

The fragment shader is slightly more complicated. We need to mix the ambient light, and the irradiance value with the diffuse value. The ambient light is just a constant, dim, "white" light. It gets multplied by the diffuse colour of the texture. It is only used if the user requests it to be switched on. We get the irradiance value from the irradiance texture and we multiply it by the diffuse texture. Finally, since we're doing neither HDR rendering nor specular reflections, the per-channel max value of a fragment shouldn't be higher than the given channel value of the diffuse texture so we clamp the result to that.

Finally gamma correction is applied then the fragment colour is returned.

I already described the render loop so let's look at the code:

```cpp
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
```

As you can see texture filtering is used for the lightmap optionally and clamping the lightmap texture to the edge prevents some lightmap bleeding. Let's finish the program, shall we?

```cpp
		//If framebuffer deugging is needed, check if debugInitialised == true
		if (false) {
			displayFramebufferTexture(framebufferDebugShader, quadVAO, debugTexture);
		}
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
}
```

Fin.

<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/noMultiNoFilter.png" alt="Image with artifacts" width="500">
</p>

Well not quite. But nearly. Why do we get the artifacts near the edges of the faces? And how could we solve them? I already spoiled the answer earlier but just in case, give it a bit of thought and we'll look at the solution in the next chapter. Thanks for reading!