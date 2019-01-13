# Chapter 4 - Shooter selection

Welcome back! As promised, today we'll look at the first part of the radiosity iteration - the shooter selection. This part consists of two functions, the first is:

```cpp
unsigned int selectShooterMesh(ObjectModel& model, ShaderLoader& shooterMeshSelectionShader, unsigned int& screenAlignedQuadVAO);
```

As described previously, we're going to select the mesh with the highest radiant energy as our shooter and we shoot from every patch on the mesh. To do this we're going to use a GPU Z-buffer sort. For this we are going to need a `screenAlignedQuadVAO`. This is simply a VAO that maps to the whole framebuffer so let's declare and initialise it before the function like this:

```cpp
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

In the float array the first two numbers in a line are the vertex coordinates, the second two are the texture coordinates. You could imagine this as if we drew two triangles to the screen that make up a square that maps to the whole framebuffer. Now we are going to render this square for every mesh in the scene so let's see how that works. The code below is the implementation of the `selectShooterMesh` function.

First, let's create a framebuffer for the shooter mesh selection draw operations:

```cpp
unsigned int shooterSelectionFramebuffer;

glGenFramebuffers(1, &shooterSelectionFramebuffer);
glBindFramebuffer(GL_FRAMEBUFFER, shooterSelectionFramebuffer);
```

Then let's create and bind the textures we need. Do note the size of the textures:

```cpp
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
```

We are going to use the RGB version of the mesh ID as the colour we draw into the framebuffer. The ID is simply the index of the mesh within the vector that stores every mesh within the Model object.

Next we set the viewport to...

```cpp
glViewport(0, 0, 1, 1);
```

...yes, to 1x1, like our textures. For a Z-buffer sort we want to render every mesh to the same pixel, however, with a different colour value (our RGB mesh ID) and a different Z-buffer value (the average energy within the radiance texture of every mesh).

Before we start the render loop, a few other things:

```cpp
glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

shooterMeshSelectionShader.useProgram();
glBindVertexArray(screenAlignedQuadVAO);
```

Let's start the render loop:

```cpp
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
```

We use the same algorithm as we did with the triangle ID RGB encoding, however, instead of using 0-255 only the 0-99 range is used for every colour and when mapping the final values to the 0-1 float range the values are limited from 0 to 0.99, with two decimal points max. The purpose of this is twofold: the first is that it is easier to debug when working with a limited set of floating point numbers and it still gives a high amount of possible meshes within a scene (100 \* 100 \* 100) and it also avoids any possible floating point precision issues...well, mostly. More on that later.

Next we'll create a texture with a mipmap pyramid out of the radiance data of the current mesh:

```cpp
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
```

Finally for this for loop, let's send the draw call then delete the texture we just created:

```cpp
    //Draw a full, screen-aligned quad
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDeleteTextures(1, &mipmappedRadianceTexture);
}
```

The function isn't done yet but we should take a look at the shader program we're using here:

ShooterMeshSelection.vs

```GLSL
#version 450 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 inTextureCoord;

out vec2 textureCoord;

void main() {
    gl_Position = vec4(position, 0.0f, 1.0f);
    textureCoord = inTextureCoord;
}
```

As you can see this is a very simple, almost pass-through shader. We're drawing the two triangles (one quad, really) that we specified and also pass on the texture coordinates.

The fragment shader is slightly more complicated. Let's look at it in a few parts:

ShooterMeshSelection.fs
```GLSL
#version 450 core

layout (location = 0) out vec3 shooterMeshID;

in vec2 textureCoord;

uniform vec3 meshID;
uniform sampler2D mipmappedRadianceTexture;

void main() {
    //Check how many mipmap levels there are
    int mipmapLevel = textureQueryLevels(mipmappedRadianceTexture);
```

So far the only interesting thing is the `textureQueryLevels`. It returns the number of mipmap levels a texture has but is limited to OpenGL 4.3 and above, so be careful if you're targetting a lower version than that.

We need this function so we know which mipmap level to sample - we need the top, 1x1 level for our function since mipmapping averages the values found in a texture so the top level contains the average of the whole texture.

We retrieve that value this way:

```GLSL
    vec3 averageEnergyVector = textureLod(mipmappedRadianceTexture, textureCoord, mipmapLevel).rgb;
```

We calculate luminance (basically the energy stored within an RGB value - some colours contribute more than others) according to page 43 of http://sirkan.iit.bme.hu/~szirmay/gpugi1.pdf. If you want to know where these values come from, they are roughly the per-column averages of the CIE XYZ colour space to RGB conversion matrix. Take a look here: https://en.wikipedia.org/wiki/CIE_1931_color_space#Construction_of_the_CIE_XYZ_color_space_from_the_Wright%E2%80%93Guild_data

In code form:

```GLSL
    float luminance = 0.21 * averageEnergyVector.r + 0.39 * averageEnergyVector.g + 0.4 * averageEnergyVector.b;
```

Finally, we have to map this value to the 0-1 float range where towards 0 the luminance value is higher. This is necessary because we then assign this value as the fragment's depth. If we don't modify which `glDepthFunc` we use (by default it is `GL_LESS`) then smaller values are going to overwrite higher values. Thus the texture with the highest luminance is going to be closest to 0 and thus the framebuffer is going to contain its RGB mesh ID in the end.

So let's do the thing I just described:

```GLSL
    gl_FragDepth = 1 / (1 + luminance);

    shooterMeshID = meshID;
}
```

That wraps up the shader code for today, however, we need to go back to the C++ side. After the render loop is done we need to read the value that was written in the end and we need to decode that value. Recall that we mapped these values to 0-0.99 so we need to multiply by 100 even in the case of the red value. This gives us the formula: 100 * R + 100 * 100 * G + 100 * 100 * 100 * B = original ID.

Let's look at the code:

```cpp
//Read back the chosen mesh ID
std::vector<GLfloat> shooterMeshID(1 * 1 * 3);

glReadBuffer(GL_COLOR_ATTACHMENT0);
glReadPixels(0, 0, 1, 1, GL_RGB, GL_FLOAT, &shooterMeshID[0]);

unsigned int chosenShooterMeshID = 0;

chosenShooterMeshID += std::round(100 * 100 * 100 * shooterMeshID[2]);
chosenShooterMeshID += std::round(100 * 100 * shooterMeshID[1]);
chosenShooterMeshID += std::round(100 * shooterMeshID[0]);
```

First we create the necessary vector to read back the written value, which is only a 3 element vector where index 0 is going to store the R value and 1 and 2 are going to store G and B, respectively.

We also need to utilise the `std::round()` function since otherwise I _still_ encountered a floating point precision issue - 0.59 times 100 was 58 according to the program without rounding, which could be "exhibit A" for off-by-one errors. Note that it **only** happened with 0.59, not with any other numbers so that was a rather annoying bug to fix. But anyways, if you round it you're going to be just fine.

That's basically it for this function, reset everything to normal values, delete whatever you need to delete and return the `chosenShooterMeshID`:

```cpp
glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
glBindFramebuffer(GL_FRAMEBUFFER, 0);
glBindVertexArray(0);
glUseProgram(0);

glDeleteFramebuffers(1, &shooterSelectionFramebuffer);
glDeleteRenderbuffers(1, &depth);
glDeleteTextures(1, &meshIDTexture);

return chosenShooterMeshID;
```

Nope, if you think we're done then I've got some bad news for you. This function gives us the scope of our iteration (the shooting mesh) but as previously mentioned, we also have sub-iterations that handle individual shooting patches, which means that we have to find our next shooting patch on the mesh.

The declaration of that function looks like this:

```cpp
void selectMeshBasedShooter(ObjectModel& model, glm::vec3& shooterRadiance, glm::vec3& shooterWorldspacePos, glm::vec3& shooterWorldspaceNormal, glm::vec2& shooterUV, unsigned int& shooterMeshIndex);
```

This function is slightly different than the previous ones. It returns the data about the next shooter patch, however, it would require multiple return statements (since we need a fair amount of data about the shooter). We can work around that by using reference parameters. We are writing to the `shooterRadiance`, `shooterWorldspacePos`, `shooterWorldspaceNormal` and the `shooterUV` parameters. We are also using the scene model and the shooterMeshIndex (produced by the first function in this chapter). 

First, we keep track of our current shooter index and shooter mesh index in a global variable, also keep track if we have run out of shooters on the current mesh and need to select a new one:

```cpp
//Need to select a mesh when the iteration first starts
bool meshSelectionNeeded = true;
unsigned int shooterIndex = 0;
unsigned int shooterMesh = 0;
```

After this, we can start with the implementation of the function. First we get the first (Red colour channel) index of the shooting patch/texel from the `texturespaceShooterIndices` vector of the shooter mesh:

```cpp
void Renderer::selectMeshBasedShooter(ObjectModel& model, glm::vec3& shooterRadiance, glm::vec3& shooterWorldspacePos, glm::vec3& shooterWorldspaceNormal, glm::vec2& shooterUV, unsigned int& shooterMeshIndex) {
	unsigned int texelIndex = model.meshes[shooterMeshIndex].texturespaceShooterIndices[shooterIndex];
```

This is useful because it allows us to easily get data from the different data vectors that we stored in every mesh as the `texelIndex` represents the same patch/texel in every data vector.

Bit of frontend-related code again:

```cpp
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
```

Basically, if the attenuation is set to linear or quadratic we don't take the area of the texel into account, otherwise we multiply the value by the texel area. The rest of a line only retrieves the radiance data per channel of the texel then scales it down by the amount of patches that are actually present on the shooter mesh. This is needed because otherwise our scenes would end up being way too bright. You can think of this as a kind of size correction - each small patch is going to shoot only a bit of the energy found in the shooting mesh.

Next we retreive the rest of the shooter information we need:

```cpp
shooterWorldspacePos = glm::vec3(model.meshes[shooterMeshIndex].worldspacePosData[texelIndex], model.meshes[shooterMeshIndex].worldspacePosData[texelIndex + 1], model.meshes[shooterMeshIndex].worldspacePosData[texelIndex + 2]);

shooterWorldspaceNormal = glm::vec3(model.meshes[shooterMeshIndex].worldspaceNormalData[texelIndex], model.meshes[shooterMeshIndex].worldspaceNormalData[texelIndex + 1], model.meshes[shooterMeshIndex].worldspaceNormalData[texelIndex + 2]);

shooterUV = glm::vec3(model.meshes[shooterMeshIndex].uvData[texelIndex], model.meshes[shooterMeshIndex].uvData[texelIndex + 1], model.meshes[shooterMeshIndex].uvData[texelIndex + 2]);
```

We also need to set the radiance of the shooter to 0 - since the shooter patch shoots all its energy in a sub-iteration:

```cpp
model.meshes[shooterMeshIndex].radianceData[texelIndex] = 0.0f;
model.meshes[shooterMeshIndex].radianceData[texelIndex + 1] = 0.0f;
model.meshes[shooterMeshIndex].radianceData[texelIndex + 2] = 0.0f;
```

Finally, we check if we need to select a new mesh or if we just need to iterate the `shooterIndex`. We need to select a new mesh if our (0-indexed) `shooterIndex` reaches the end of the shooter indices our mesh has:

```cpp
if (shooterIndex >= model.meshes[shooterMeshIndex].texturespaceShooterIndices.size() - 1) {
    meshSelectionNeeded = true;

    shooterIndex = 0;
}
else {
    ++shooterIndex;
}
```

Phew, that was a handful but we're done for now. Congratulation for making it this far, our renderer is coming together piece by piece. Next time we'll start the form factor calculation by taking a look at visibility calculation...with hemicubes! Thanks for reading and stay tuned!