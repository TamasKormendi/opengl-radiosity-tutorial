# Chapter 2 - The preprocess step

Hi everyone, today we are going to look at the very first step in our radiosity implementation - the preprocess step. The function declaration looks like this:

```cpp
void preprocess(ObjectModel& model, ShaderLoader& shader, glm::mat4& mainObjectModelMatrix);
```

### What is this?

This step renders the geometric information of the whole scene (sort of like a global G-buffer) in texture-space and stores this information for later steps.

### Why do we need this? 

The answer to this is quite simple: we need geometric data (world-space position and normal) about our shooter. When we are rasterising our scene to transfer energy from the shooter to the receivers we have information about each individual receiver in the shaders, however, we don't have this luxury in the case of the shooter since they are going to be `uniform` values for a whole sub-iteration. This function makes those values easily available when we need them.

### How are we going to do this?

We basically have to render the different geometric information into a few different textures. Fortunately this is not hard to do with OpenGL's multiple render targets feature.

However, the function does a few other things too so let's look at it in detail. First, the parameters:

* model: the object model representing the whole scene (for more info see https://learnopengl.com/Model-Loading/Model)
* shader: the shader program (vertex+fragment) we use for this function
* mainObjectModelMatrix: the model matrix for the main model of the scene

The first part of the function is rather straightforward: create a framebuffer and four textures to store the result of the texture-space draw calls. We also set the size of the textures to `RADIOSITY_TEXTURE_SIZE` which is, as the name implies, the size of our irradiance and radiance textures and switch off texture filtering with:

```GLSL
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
```

At this point you might notice a few strange things, namely the `idData` and `uvData` textures. Bit of a spoiler, but `uvData` isn't used in the current renderer. The purpose of that would be to enable us to have a GPU-only shooter selection, instead of the GPU-CPU hybrid in place now. The `idData` is repurposed from an earlier version of the renderer (which was following https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter39.html). For this every mesh was decomposed into Triangle objects and every triangle was given a unique RGB ID. You might think that this feature is irrelevant now, however, I repurposed the fact that I have an easy way to mark texels that are not blank in texture-space with values that are guaranteed to be above (0, 0, 0) in order to introduce some optimisations. More about that (and the Triangle class itself) later.

After creating the textures simply configure our framebuffer so we can write to 4 targets:

```cpp
unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

glDrawBuffers(4, attachments)
```

Then - since we're rendering in texture-space - set the size of the viewport to the size of the radiosity textures and set the clear colour:

```cpp
glViewport(0, 0, ::RADIOSITY_TEXTURE_SIZE, ::RADIOSITY_TEXTURE_SIZE);

glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
```

The next part deals with rendering the scene, one mesh at a time. You'll see similar "render blocks" throughout the code. They could be abstracted away into another function for less code repetition, however, that'd make it slightly more complicated to explain the code so do excuse me for keeping them as is.

The first line is a weird one:

```cpp
int lampCounter = 0;
```

What does this do? Well, the renderer supports an arbitrary number of arbitrarily placed lights. The way this is done is by keeping track of the position of each lamp in a vector `lightLocations` and adding a lamp mesh to the main Model every time the user adds a lamp. This lamp counter is used to retrieve the correct location of every lamp from the aforementioned vector.

The following for loop pretty much concludes the function, however, it is a rather long one so let's break it up a bit.

First, let's loop through every mesh within the Model, clear the buffer and use the shader program we passed to the function as a parameter:

```cpp
for (unsigned int i = 0; i < model.meshes.size(); ++i) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.useProgram();
```

Later we are going to need the texture-space data we render in the upcoming render pass. Since we'll need access to the data on the CPU, we have to read it back from the GPU. We have to create vectors that are big enough for our purpose, that is, the square of the `RADIOSITY_TEXTURE_SIZE` multiplied by 3, since we are reading back RGB values:

```cpp
    //Create readback buffers for all of the texture data
    std::vector<GLfloat> worldspacePositionDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
    std::vector<GLfloat> normalVectorDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);

    std::vector<GLfloat> idDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
    std::vector<GLfloat> uvDataBuffer(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3);
```
Next, we render the current mesh in texture-space but with world-space geometric data. If the current mesh is a lamp we'll translate it according to its position and we scale it, if desired. Don't forget to increment the `lampCounter` either! Otherwise we use the Model matrix for the main Model:
```cpp
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
```

At this point it might make sense to take a bit of a break from the CPU C++ code and take a look at the shader code we're using in the previous block.

First, the vertex shader. It is a rather standard vertex shader, with one oddity. Let's see if you can spot it:

preprocess.vs
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

uniform mat4 model;
void main() {
    fragPos = vec3(model * vec4(vertexPos, 1.0));

    normal = mat3(transpose(inverse(model))) * inNormal;

    textureCoord = inTextureCoord;

    ID = inID;

    //The texture coordinates are originally in [0-1], we need to make them [-1, 1]
    vec2 textureSpace = (inTextureCoord - 0.5) * 2;

    gl_Position = vec4(textureSpace.x, textureSpace.y, 0.5, 1.0);
}
```

Yep, when we're assigning to the `gl_Position` we don't do it by multiplying a value with different matrices. Since we're rendering in texture-space we are writing the `gl_Position` according to our texture coordinates. You might recall that texture coordinates range from 0 to 1 float (inclusive) but `gl_Position` expects coordinates in clip-space, which ranges from -1 to 1 float (inclusive). So we need to map the texture-space coordinates to clip-space. This is rather easy, just subtract 0.5 from the original texture coordinates then multiply the result by 2, as seen here:

```GLSL
vec2 textureSpace = (inTextureCoord - 0.5) * 2;
```

The z coordinate of `gl_Position` doesn't matter too much (I just assigned 0.5 to it) and the w coordinate should be 1.0, since we absolutely don't want any perspective distortion.

The fragment shader is basically a pass-through shader, the only interesting part of it is that we're writing to multiple textures:

preprocess.fs
```GLSL
#version 450 core

layout (location = 0) out vec3 worldspacePosition;
layout (location = 1) out vec3 worldspaceNormal;
layout (location = 2) out vec3 triangleID;
layout (location = 3) out vec3 interpolatedUV;

in vec3 fragPos;
in vec3 normal;
in vec2 textureCoord;
in vec3 ID;

void main() {

    worldspacePosition = fragPos;

    //If we ever need non-normalised normals, look here
    worldspaceNormal = normalize(normal);

    triangleID = ID;

    interpolatedUV = vec3(textureCoord, 0.0);

}
```
That concludes our shader code, for now. Where were we? Oh, yes, the render loop of the preprocess function. There's not much left for this tutorial, I promise, so hang in there.

After we're done with the texture-space render pass we have to read all of the textures we just produced back to the CPU. We can do this with the `glReadPixels()` function. First we read the values back to the intermediate vectors then once all is said and done we swap the data vectors stored within the mesh object to the just written buffers. If you want to optimise the code a bit you can most likely leave out the buffer vectors and write straight to the vectors stored in the mesh.

```cpp
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
```

(Nearly) finally, a bit of an optimisation. Due to us rendering in texture-space, not every mesh might map to every texel within a texture. In other words, if a texel is left blank, it cannot hold data about a shooter and thus it can be safely disregarded. The next code block loops through all the RGB idData and if the sum of the RGB value corresponding to a texel is above 0 it adds the starting index (so the index of the red value) to a vector that stores the index of every possible shooter for a given mesh:

```cpp
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
```

I was tempted to leave out this last code block for now, since it also partly deals with settings passed from the frontend. What you need to know for now is that the area of a patch (texel) also influences how much energy it shoots back into the scene. Since this program is a general-purpose renderer it doesn't define a unit system and might make sense to consider every patch uniform size. However, I also gave the users a way to make patches that are significantly bigger have a higher amount of impact on the scene. For this to work the average texel area is calculated per mesh:

```cpp
model.meshes[i].overallArea / model.meshes[i].texturespaceShooterIndices.size();
```

...and 1.0 is added to it, to prevent downscaling and thus prevent scenes that are unreasonably dark. If the user doesn't choose this, just use unit (1.0) size for every patch:

```cpp
    if (attenuationType == static_cast<int>(ATTENUATION::ATT_LINEAR) || attenuationType == static_cast<int>(ATTENUATION::ATT_QUAD)) {
        model.meshes[i].texelArea = 1.0;
    }
    else {
        //1.0 + ... is to prevent downscaling, which usually makes scenes way too dark
        model.meshes[i].texelArea = 1.0 + model.meshes[i].overallArea / model.meshes[i].texturespaceShooterIndices.size();
    }
}
```

In the final part of the function feel free to delete the textures or the framebuffer, but at least don't forget to reset the viewport and the framebuffer we are using:

```cpp
//Delete textures here if they are not needed anymore
glReadBuffer(GL_COLOR_ATTACHMENT0);
glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

...and that's it! I can see that you might still have questions about a few parts ("Since when does the Mesh class have a `texturespaceShooterIndices` vector or how does the Triangle class even work?") but I feel that this chapter is enough to digest as is, so we'll look at the Triangle class and the modifications to the object loading classes in the next tutorial.

Thanks for reading!
