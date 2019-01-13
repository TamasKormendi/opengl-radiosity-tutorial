# Chapter 6 - Lightmap update

Welcome back! This is the last part of our radiosity algorithm so let's get right into it, shall we?

In order to update the ir/radiance values stored in our lightmaps, as you guessed, we'll need a render pass to do so. The general strategy here is to bind our old ir/radiance maps (so we can retrieve values from them for the render pass) then render into two framebuffers (for the new radiance and irradiance textures). The calculated value for these is going to be the value from the old ir/radiance textures and the delta ir/radiance, which we are going to calculate within our shader program. Here you'll also see how the geometric term is calculated.

Okay, let's start! The function declaration is as follows:

```cpp
void updateLightmaps(ObjectModel& model, ShaderLoader& lightmapUpdateShader, glm::mat4& mainObjectModelMatrix, std::vector<glm::mat4>& viewMatrices, std::vector<unsigned int>& visibilityTextures);
```

The parameters are quite standard, however, we also have to pass the view matrices and visibility textures calculated in `createHemicubeTextures()` so don't forget that.

The first part of the function simply creates a framebuffer and two textures, one for the new irradiance data and one for the new radiance data:

```cpp
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
```

So far so standard, right? After this, we'll have to set the view matrices and the visibility textures produced by our hemicube function:

```cpp
//Set the view matrices
lightmapUpdateShader.useProgram();

lightmapUpdateShader.setUniformMat4("view", viewMatrices[0]);

lightmapUpdateShader.setUniformMat4("leftView", viewMatrices[1]);
lightmapUpdateShader.setUniformMat4("rightView", viewMatrices[2]);

lightmapUpdateShader.setUniformMat4("upView", viewMatrices[3]);
lightmapUpdateShader.setUniformMat4("downView", viewMatrices[4]);

lightmapUpdateShader.setUniformInt("attenuationType", attenuationType);

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

//Set the active texture to GL_TEXTURE0 afterwards
glActiveTexture(GL_TEXTURE0);
```

Our render loop is going to be a bit different than usual. We calculate the lightmap values for a single mesh in one iteration then read this data back to the CPU. The problem with this method is that it has to make `glReadPixels()` calls during every iteration so this can slow down the pipeline and create a bottleneck. The upside of this method, however, is that it is rather GPU memory efficient - we only store the new lightmaps for the current mesh. The other way of doing it would be to calculate the new lightmap values for every mesh before reading them back to the CPU. This is most likely the faster method since `glReadPixels()` calls happen in one block, but we'd have to store all of the new lightmaps in GPU memory. I would encourage you to give this a shot and see how the results differ. For now, we'll stick to the "render mesh - update lightmaps" method instead of the "render scene - update lightmaps" one:

```cpp
int lampCounter = 0;

	for (unsigned int i = 0; i < model.meshes.size(); ++i) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		std::vector<GLfloat> newIrradianceDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
		std::vector<GLfloat> newRadianceDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);

		lightmapUpdateShader.setUniformFloat("texelArea", model.meshes[i].texelArea);

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
```

This is not the whole loop, but at this point it might be worth examining our shader programs.

First, we are using a sizeable amount of input and output variables and uniforms, so let's declare them:


LightmapUpdate.vs
```GLSL
#version 450 core

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTextureCoord;
layout (location = 3) in vec3 inID;

out vec3 fragPos;
out vec3 normal;
out vec2 textureCoord;
out vec3 ID;

out vec4 fragPosLightSpace;
out vec3 normalLightSpace;

out vec4 fragPosLeftLightSpace;
out vec4 fragPosRightLightSpace;
out vec4 fragPosUpLightSpace;
out vec4 fragPosDownLightSpace;

out vec3 cameraspace_position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 leftView;
uniform mat4 rightView;
uniform mat4 upView;
uniform mat4 downView;
```

Next, in our main function there are two noteworthy things: first, as this is a lightmap update pass this render pass is going to be in texture-space. Second, the `cameraspace_position` in this case is going to belong to the camera of the shooter. In other words, it is going to represent the scene through the front face of the hemicube placed on the shooter. It is the same idea that our normal camera uses, just with a different view matrix. Oh, also keep in mind that the five different `fragPosLightSpace` variables represent the scene through the five faces of the hemicube placed on the shooter, but in clip-space:

```GLSL
void main() {
    fragPos = vec3(model * vec4(vertexPos, 1.0));

    normal = mat3(transpose(inverse(model))) * inNormal;

    normalLightSpace = mat3(transpose(inverse(view * model))) * inNormal;

    textureCoord = inTextureCoord;

    ID = inID;

    cameraspace_position = vec3(view * model * vec4(vertexPos, 1.0));

    fragPosLightSpace = projection * view * model * vec4(vertexPos, 1.0);

    fragPosLeftLightSpace = projection * leftView * model * vec4(vertexPos, 1.0);
    fragPosRightLightSpace = projection * rightView * model * vec4(vertexPos, 1.0);
    fragPosUpLightSpace = projection * upView * model * vec4(vertexPos, 1.0);
    fragPosDownLightSpace = projection * downView * model * vec4(vertexPos, 1.0);

    //The texture coordinates are originally in [0-1], we need to make them [-1, 1]
    vec2 textureSpace = (inTextureCoord - 0.5) * 2;

    gl_Position = vec4(textureSpace.x, textureSpace.y, 0.5, 1.0);
}
```

In our fragment shader we have to declare a fair amount of input and output variables and a few uniforms:

LightmapUpdate.fs
```GLSL
#version 450 core

layout (location = 0) out vec3 newIrradianceValue;
layout (location = 1) out vec3 newRadianceValue;

in vec3 fragPos;
in vec3 normal;
in vec2 textureCoord;
in vec3 ID;

in vec3 cameraspace_position;
in vec3 normalLightSpace;

in vec4 fragPosLightSpace;

in vec4 fragPosLeftLightSpace;
in vec4 fragPosRightLightSpace;
in vec4 fragPosUpLightSpace;
in vec4 fragPosDownLightSpace;

uniform vec3 shooterRadiance;
uniform vec3 shooterWorldspacePos;
uniform vec3 shooterWorldspaceNormal;
uniform vec2 shooterUV;

uniform sampler2D irradianceTexture;
uniform sampler2D radianceTexture;

uniform sampler2D visibilityTexture;

uniform sampler2D leftVisibilityTexture;
uniform sampler2D rightVisibilityTexture;
uniform sampler2D upVisibilityTexture;
uniform sampler2D downVisibilityTexture;

uniform sampler2D texture_diffuse0;

uniform bool isLamp;

uniform float texelArea;
uniform int attenuationType;
```

Afterwards, let's define a function that performs the shadow mapping procedure. It returns a float, since we use percentage-closer filtering (PCF) with 25 samples in order to smooth out the shadow edges a bit:

```GLSL
//Parts of this function adapted from https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mappings
float isVisible(sampler2D hemicubeFaceVisibilityTexture, vec4 hemicubeFaceSpaceFragPos) {

    //Perform the perspective divide and scale to texture coordinate range
    vec3 projectedCoordinates = hemicubeFaceSpaceFragPos.xyz / hemicubeFaceSpaceFragPos.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;

    float closestDepth = texture(hemicubeFaceVisibilityTexture, projectedCoordinates.xy).r;
    float currentDepth = projectedCoordinates.z;

    float shadow = 0.0;

    //Use percentage-closer filtering (PCF), 25 samples
    vec2 texelSize = 1.0 / textureSize(hemicubeFaceVisibilityTexture, 0);
    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(hemicubeFaceVisibilityTexture, projectedCoordinates.xy + vec2(x, y) * texelSize).r; 
            shadow += (currentDepth) > pcfDepth ? 0.0 : 1.0;        
        }    
    }

    shadow = shadow / 25.0;

    return shadow;
}
```

We'll call this function in `main()`. For now, we'll just use the front face so we can debug it easier if we happen to face any issues:

```GLSL
void main() {
    float isFragmentVisible = 0.0;

    float centreShadow = isVisible(visibilityTexture, fragPosLightSpace);

    isFragmentVisible = centreShadow;
```

We'll add the other faces later. Afterwards, we can calculate the geometric term. For this, we need to calculate the `cos` of two angles. The angle between the normal of the shooter and the vector between the shooter and the receiver AND the angle between the normal of the receiver and the vector between the shooter and the receiver. Let's call these `cosThetaY` and `cosThetaX` respectively. Let's declare a few variables then calculate `cosThetaX`. Also keep in mind that we'll work with normalised vectors (aside from the distance calculation):

```GLSL
    const float pi = 3.1415926535;

    //Form factor between shooter and receiver
    float Fij = 0.0;

    //Adapted from http://sirkan.iit.bme.hu/~szirmay/gpugi1.pdf
    //(Previous versions of this part also used https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter39.html
    //but by now only a few variable names might remain)

    //Calculations are done in camera-space position, if the hemicube front is considered the camera

    //A vector from the shooter to the receiver
    vec3 shooterReceiverVector = normalize(cameraspace_position);
    //This might not be necessary - just in case the normal vectors aren't normalised
    vec3 normalisedNormalLightSpace = normalize(normalLightSpace);

    float distanceSquared = dot(cameraspace_position, cameraspace_position);
    float distanceLinear = length(cameraspace_position);

    float cosThetaX = dot(normalisedNormalLightSpace, -shooterReceiverVector);
    if (cosThetaX < 0) {
        cosThetaX = 0;
    }
```

You can visualise the `cosThetaX` calculation by drawing a vector from the shooter to the normal vector of the receiver (maybe even in 2D). If you draw the directional arrows you would see that the two vectors point toward each other. To easily calculate the `cos` between the vectors, we reverse the `shooterReceiverVector`. In other words we take the dot product of `normalisedNormalLightSpace` and `-shooterReceiverVector`. We set the result to 0 if it ends up being negative.

The idea is similar with `cosThetaY`, however, we don't have to reverse any vectors. Also keep in mind that the calculations are in camera-space (or more like hemicube-front-face-space) so we know that the direction vector of the shooter is negative Z: (0, 0, -1):

```GLSL
    //In camera-space the shooter looks along negative Z
    vec3 shooterDirectionVector = vec3(0, 0, -1);

    float cosThetaY = dot(shooterDirectionVector, shooterReceiverVector);
    if (cosThetaY < 0) {
        cosThetaY = 0;
    } 
```

If you're reading the GPUGI article (and if you aren't you really should, it's pretty good), you might be wondering why they use (0, 0, 1) as the shooter normal and only the z-coordinate of the "ytox" (for us `shooterReceiverVector`) as the `cosThetaY`. The answer is that that article uses DirectX/HLSL so the forward direction for a camera is positive Z. In that case it is possible to just take the Z-element of the `shooterReceiverVector` and use that as the `cosThetaY`, since the dot product is going to be the product of (0, 0, 1) and the `shooterReceiverVector`, which is equal to `shooterReceiverVector.z`.

Next we'll calculate the geometric term since we have all the information we need now. The way we calculate it is the radiosity form factor calculation. The one where we take the texel area into account is the full formula, the others only use linear or squared distance as the normalisation factor:

```GLSL
    //This conditional avoids division by 0
    if (distanceSquared > 0) {
        float geometricTerm = 0.0;

        if (attenuationType == 0) {
            geometricTerm = cosThetaX * cosThetaY / distanceLinear;
        }
        else if (attenuationType == 1) {
            geometricTerm = cosThetaX * cosThetaY / distanceSquared;
        }
        else if (attenuationType == 2) {
            geometricTerm = (cosThetaX * cosThetaY / (distanceSquared * pi)) * texelArea;
        }

        Fij = geometricTerm;
    }
    else {
        Fij = 0;
    }
```

Afterwards, we multiply by the visibility term, sample our old ir/radiance textures and the diffuse texture:

```GLSL
    Fij = Fij * isFragmentVisible;

    vec3 oldIrradianceValue = texture(irradianceTexture, textureCoord).rgb;
    vec3 oldRadianceValue = texture(radianceTexture, textureCoord).rgb;

    vec3 diffuseValue = texture(texture_diffuse0, textureCoord).rgb;
```

Since we passed the `shooterRadiance` as a uniform we can calculate the delta irradiance. Our delta radiance is going to be this value multiplied by the diffuse texture:

```GLSL
    vec3 deltaIrradiance = Fij * shooterRadiance;
    vec3 deltaRadiance = deltaIrradiance * diffuseValue;
```

The only thing that remains is to add the delta ir/radiance to the old ir/radiance values. I also have to mention that lamps do not accumulate - this is to avoid an issue I was having where (if a lamp was placed too close to a wall) the lamp and the wall "ping-ponged" between each other - that is, these two meshes always had the highest radiance and did not let other meshes shoot for quite a long time:

```GLSL
    //Lamps only shoot but do not accumulate - needed to eliminate a back-and-forth selection issue between lamp and a close wall
    if (isLamp) {
        newIrradianceValue = oldIrradianceValue;
        newRadianceValue = oldRadianceValue;
    }
    else {
        newIrradianceValue = oldIrradianceValue + deltaIrradiance;
        newRadianceValue = oldRadianceValue + deltaRadiance;
    }
}
```

On the CPU-side we just have to read back the produced values and after the loop is finished, clean up:

```cpp
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
```

That's it! I know you can't run the program properly just yet, but if you did everything correctly this is where you should be right now. Congratulations, we're done with most of the wor...oh wait a minute...something doesn't look right...

<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/hemicube_front_3_bugs.png" alt="Image with 3 bugs" width="500">
</p>

This is not supposed to happen, is it?


The answer is, no, it isn't. Before you go looking through every single line of code multiple times, trying to find the bug (like I did when I encountered this scene for the first time) I'll go ahead and spoil it that there's nothing wrong with our code. Well, there is, but it is due to a lack of a few lines of code rather than us having made a mistake in the existing codebase. Let's go and find out what we're missing, shall we?

First, what you see above is a nasty mixture of 3 bugs. In no particular order, let's fix them. Leading the pack is going to be the barcode-like artifacts all over the scene. Believe it or not, those bars are not more than a really common shadow mapping error, shadow acne. Because of our use of lightmaps instead of the normal acne-like appearance it took on a form of black bars. The fix is now easy, we need to revisit the `isVisible()` function to add a bit of bias and modify how we check the depth map:

```GLSL
float bias = 0.001;

//Rest of the function...

vec2 texelSize = 1.0 / textureSize(hemicubeFaceVisibilityTexture, 0);
for(int x = -2; x <= 2; ++x) {
	for(int y = -2; y <= 2; ++y) {
		float pcfDepth = texture(hemicubeFaceVisibilityTexture, projectedCoordinates.xy + vec2(x, y) * texelSize).r; 
		shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;        
	}    
}
```

The bias here is nothing complicated but works generally well for the Cornell-box-like test scene. If you want to be more fancy with it (i.e. modify it depending on the angle between the surface and the light emitter), feel free to!

The scene now looks like this: 


<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/hemicube_front_2_bugs.png" alt="Image with 2 bugs" width="500">
</p>

The second issue is called oversampling. When we sample the depth map some fragments are sampled outside the range of the depth map. The depth values retrieved in cases like this by default is maximum depth, which means that these fragments are always going to be marked visible. Again, the fix is not hard: specify a border "colour" (in this case more like border depth) of minimum depth in the function where you create the hemicube then clamp the depth map to this border. With this fix the depth map is declared and initialised this way:

```cpp
float borderColour[] = { 0.0f, 0.0f, 0.0f, 1.0f };

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
```

Getting there:

<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/hemicube_front_1_bug.png" alt="Image with 1 bug" width="500">
</p>


There is still an annoying bar of light where it should not be, directly in the plane of the light source. It is caused by the fragments that are in the aforementioned plane having a depth value of 0 compared to the light emitter (since they are in the same plane). Once again, let's revisit the `isVisible() `function and append a small section to it, that adds our bias to the fragment and checks if it maps to the plane of the shooter (i.e. 0.0) or behind that point:

```GLSL

//Rest of the function...


shadow = shadow / 25.0;

float zeroDepth = currentDepth - bias;

//If the current fragment is in one plane or behind the shooter, it is in shadow
if (zeroDepth <= 0) {
    //Zero means it is in shadow
    shadow = 0.0;
}    

return shadow;
```

...and we're done:

<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/hemicube_front_0_bugs_hopefully.png" alt="Bug-free zone" width="500">
</p>

A small note: my renderer didn't have PCF implemented when these screenshots were taken so do excuse the harder shadow edges.

All right, this was a rather heavy tutorial. I'll post the whole fixed hemicube function (with a secondary solution in comments) and the fixed fragment shader (with all the hemicube faces used) below. Next time we'll put everything together and see our whole algorithm in action. Until then, stay tuned!

## Futher reading

If you want to know more about the theory behind this tutorial here's the original paper about the radiosity hemicube: http://artis.imag.fr/~Cyril.Soler/DEA/IlluminationGlobale/Papers/p31-cohen.pdf

If you need a reminder about how traditional shadow mapping works take a look here: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping


## Corrected code

The hemicube function:

```cpp
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

    	/*Shorter solution:
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


	Instead of the code above, this avoids a bunch of repetition but might be a bit harder to understand so I'll leave both
	*/
}
```

And the fragment shader:

LightmapUpdate.fs
```GLSL

#version 450 core

layout (location = 0) out vec3 newIrradianceValue;
layout (location = 1) out vec3 newRadianceValue;

in vec3 fragPos;
in vec3 normal;
in vec2 textureCoord;
in vec3 ID;

in vec3 cameraspace_position;
in vec3 normalLightSpace;

in vec4 fragPosLightSpace;

in vec4 fragPosLeftLightSpace;
in vec4 fragPosRightLightSpace;
in vec4 fragPosUpLightSpace;
in vec4 fragPosDownLightSpace;

uniform vec3 shooterRadiance;
uniform vec3 shooterWorldspacePos;
uniform vec3 shooterWorldspaceNormal;
uniform vec2 shooterUV;

uniform sampler2D irradianceTexture;
uniform sampler2D radianceTexture;

uniform sampler2D visibilityTexture;

uniform sampler2D leftVisibilityTexture;
uniform sampler2D rightVisibilityTexture;
uniform sampler2D upVisibilityTexture;
uniform sampler2D downVisibilityTexture;

uniform sampler2D texture_diffuse0;

uniform bool isLamp;

uniform float texelArea;
uniform int attenuationType;

//Parts of this function adapted from https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mappings
float isVisible(sampler2D hemicubeFaceVisibilityTexture, vec4 hemicubeFaceSpaceFragPos) {
    //Add a bit of bias to eliminate shadow acne
    float bias = 0.001;

    //Perform the perspective divide and scale to texture coordinate range
    vec3 projectedCoordinates = hemicubeFaceSpaceFragPos.xyz / hemicubeFaceSpaceFragPos.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;

    float closestDepth = texture(hemicubeFaceVisibilityTexture, projectedCoordinates.xy).r;
    float currentDepth = projectedCoordinates.z;

    float shadow = 0.0;

    //Use percentage-closer filtering (PCF), 25 samples
    vec2 texelSize = 1.0 / textureSize(hemicubeFaceVisibilityTexture, 0);
    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(hemicubeFaceVisibilityTexture, projectedCoordinates.xy + vec2(x, y) * texelSize).r; 
            shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;        
        }    
    }

    shadow = shadow / 25.0;

    float zeroDepth = currentDepth - bias;

    //If the current fragment is in one plane or behind the shooter, it is in shadow
    if (zeroDepth <= 0) {
        //Zero means it is in shadow
        shadow = 0.0;
    }    

    return shadow;
}


void main() {
    float isFragmentVisible = 0.0;

    //Check how visible the fragment is from each hemicube face
    float centreShadow = isVisible(visibilityTexture, fragPosLightSpace);
    float leftShadow = isVisible(leftVisibilityTexture, fragPosLeftLightSpace);
    float rightShadow = isVisible(rightVisibilityTexture, fragPosRightLightSpace);
    float upShadow = isVisible(upVisibilityTexture, fragPosUpLightSpace);
    float downShadow = isVisible(downVisibilityTexture, fragPosDownLightSpace);

    isFragmentVisible = centreShadow + leftShadow + rightShadow + upShadow + downShadow;


    const float pi = 3.1415926535;

    //Form factor between shooter and receiver
    float Fij = 0.0;

    //Adapted from http://sirkan.iit.bme.hu/~szirmay/gpugi1.pdf
    //(Previous versions of this part also used https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter39.html
    //but by now only a few variable names might remain)

    //Calculations are done in camera-space position, if the hemicube front is considered the camera

    //A vector from the shooter to the receiver
    vec3 shooterReceiverVector = normalize(cameraspace_position);
    //This might not be necessary - just in case the normal vectors aren't normalised
    vec3 normalisedNormalLightSpace = normalize(normalLightSpace);

    float distanceSquared = dot(cameraspace_position, cameraspace_position);
    float distanceLinear = length(cameraspace_position);

    float cosThetaX = dot(normalisedNormalLightSpace, -shooterReceiverVector);
    if (cosThetaX < 0) {
        cosThetaX = 0;
    }

    //In camera-space the shooter looks along negative Z
    vec3 shooterDirectionVector = vec3(0, 0, -1);

    float cosThetaY = dot(shooterDirectionVector, shooterReceiverVector);
    if (cosThetaY < 0) {
        cosThetaY = 0;
    } 

    //This conditional avoids division by 0
    if (distanceSquared > 0) {
        float geometricTerm = 0.0;

        if (attenuationType == 0) {
            geometricTerm = cosThetaX * cosThetaY / distanceLinear;
        }
        else if (attenuationType == 1) {
            geometricTerm = cosThetaX * cosThetaY / distanceSquared;
        }
        else if (attenuationType == 2) {
            geometricTerm = (cosThetaX * cosThetaY / (distanceSquared * pi)) * texelArea;
        }

        Fij = geometricTerm;
    }
    else {
        Fij = 0;
    }

    //So far Fij only holds the geometric term, need to multiply by the visibility term
    Fij = Fij * isFragmentVisible;

    vec3 oldIrradianceValue = texture(irradianceTexture, textureCoord).rgb;
    vec3 oldRadianceValue = texture(radianceTexture, textureCoord).rgb;

    vec3 diffuseValue = texture(texture_diffuse0, textureCoord).rgb;

    vec3 deltaIrradiance = Fij * shooterRadiance;
    vec3 deltaRadiance = deltaIrradiance * diffuseValue;

    //Lamps only shoot but do not accumulate - needed to eliminate a back-and-forth selection issue between lamp and a close wall
    if (isLamp) {
        newIrradianceValue = oldIrradianceValue;
        newRadianceValue = oldRadianceValue;
    }
    else {
        newIrradianceValue = oldIrradianceValue + deltaIrradiance;
        newRadianceValue = oldRadianceValue + deltaRadiance;
    }
}
```

