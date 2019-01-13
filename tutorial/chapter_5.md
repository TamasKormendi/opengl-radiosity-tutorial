# Chapter 5 - Visibility calculation

Welcome back everyone, last time we had a look at shooter selection and now we are going to take a look at the visibility calculation part of our radiosity algorithm. Don't worry, we'll also look at the calculation of the geometric term but that happens in a future step (the _lightmap update_) so for now we're going to calculate the visibility term. If you're somewhat familiar with radiosity papers you've most likely come across the terms "hemisphere" and "hemicube". There is plenty of material about how they are supposed to work _in theory_ so if you really want to dig deep you can look them up in-depth. Here I'll cover how visibility calculation with a hemicube can be *implemented*. Personally I tried using the hemisphere method as well but the amount of artifacts produced were unacceptable, so with OpenGL using a hemicube makes more sense. For our current purposes it is enough to know that a hemicube is a cube cut in half. It can be used to check what is visible from a given patch and receivers can be checked for visibility (with the normal Z-buffer shadow mapping algorithm). Without further ado, let's begin.

We could postpone this step but it'd only make it more painful so let's dive into vectors. In order to be able to build a robust hemicube solution we need to account for patches looking in **every** direction. To provide a bit more context: we basically want to create a virtual camera for every patch (i.e. something similar to the camera we observe the scene with). When cameras are created programmers usually define a "worldUp" vector that points to positive Y `(0, 1, 0)`. This is needed so the orientation of the camera can be decided, since in 3D space there is an infinite amount of vectors that are at a right angle to the direction vector we want to look along and all of them could be the up vector. With this `worldUp` vector first a `rightVector` (which is at a right angle to the camera and as the name implies, to the right) can be calculated by taking the cross product of the direction vector and the `worldUp` vector. From this vector we can calculate the `upVector` of the camera by taking the cross product of the `rightVector` and the direction vector. Don't forget to normalise the vectors after each step. In code constructing a camera with the help of the `glm::lookAt(eyePosition, cameraCenter, upVector)` function would look like this:

```cpp
glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraPosition + directionVector, upVector);
```

Easy, right? This works for a camera but what happens if our patch has a direction vector that is parallel with our `worldUp`? If you perform the previously described steps with a vector like that (i.e. `(0, 1, 0)`) and our `worldUp` kept at `(0, 1, 0)` you are going to end up with `(0, 0, 0)` as our `upVector`. Which is not good, because the `glm::lookAt()` function's upVector cannot be `(0, 0, 0)`. This problem is further exacerbated when we are calculating the vectors for the hemicube, since we need to juggle 5 vectors (one for each face of the hemicube) and an `upVector` for each of them. We have to approach this problem in a slightly more clever way.

What if we decomposed this problem into two stages? One for handling the `worldUp` vector and another one to handle the up vector for the hemicube (the `hemicubeUp`)? Enter the "double up" technique.

The first problem, the one with the `worldUp` can be fixed rather easily. We know our first solution fails in some cases, but how often does it fail? If the direction we want to look along (or indeed, _shoot_ along, if we consider patches, which we do) is parallel with the defined `worldUp`. If we normalise the direction vector it can be seen that this happens in exactly 2 cases. If the direction vector is `(0, 1, 0)` or `(0, -1, 0)`. We can simply redefine our worldUp if one of those cases happens:

```cpp
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

//This is our normalised direction vector
glm::vec3 normalisedShooterNormal = glm::normalize(shooterWorldspaceNormal);

if (normalisedShooterNormal.x == 0.0f && normalisedShooterNormal.y == 1.0f && normalisedShooterNormal.z == 0.0f) {
	worldUp = glm::vec3(0.0f, 0.0f, -1.0f);
}
else if (normalisedShooterNormal.x == 0.0f && normalisedShooterNormal.y == -1.0f && normalisedShooterNormal.z == 0.0f) {
	worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
}
```

Using this "sanitised" `worldUp` we can define a `rightVector` and an `upVector` for the hemicube:

```cpp
glm::vec3 hemicubeRight = glm::normalize(glm::cross(shooterWorldspaceNormal, worldUp));
glm::vec3 hemicubeUp = glm::normalize(glm::cross(hemicubeRight, shooterWorldspaceNormal));
```

From this point only a few basic operations remain. For the front, left and right faces we can use the `hemicubeUp` as the `upVector`. For the direction vectors, the front face can use the normal vector of the shooter, the left face can use the negative `hemicubeRight` and the right face can use the `hemicubeRight`. This is what it looks like in code:

```cpp
glm::mat4 frontShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + shooterWorldspaceNormal, hemicubeUp);

glm::mat4 leftShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + (-hemicubeRight), hemicubeUp);
glm::mat4 rightShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + hemicubeRight, hemicubeUp);
```

Only the up and down faces remain. For these the direction vector can be the `hemicubeUp` and negative `hemicubeUp`, respectively. The up vectors can be the negative normalised direction vector and the normalised direction vector, respectively.

```cpp
glm::mat4 upShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + hemicubeUp, -normalisedShooterNormal);
glm::mat4 downShooterView = glm::lookAt(shooterWorldspacePos, shooterWorldspacePos + (-hemicubeUp), normalisedShooterNormal);
```

And we're done with the hard part! Next take a look at the function declaration:

```cpp
std::vector<unsigned int> createHemicubeTextures(ObjectModel& model, ShaderLoader& hemicubeShader, glm::mat4& mainObjectModelMatrix, std::vector<glm::mat4>& viewMatrices, unsigned int& resolution, glm::vec3& shooterWorldspacePos, glm::vec3& shooterWorldspaceNormal);
```

It is a handful but the only two notable things are: we return an `std::vector` of the unsigned ints that refer to the depth textures and since we use the view matrices we construct here in the upcoming `updateLightmaps()` function, we write to a `glm::mat4&` that contains the matrices.

Let's render the front face first. We're using a 90° projection matrix to fit together the faces of the cube perfectly. Here I decided against using a cube map so there is going to be some code repetition. This is just one solution that is possibly far from the best so feel free to use a cube map or improve it in any way if you want to.

```cpp
unsigned int hemicubeFramebuffer;
glGenFramebuffers(1, &hemicubeFramebuffer);

unsigned int frontDepthMap;
glGenTextures(1, &frontDepthMap);
glBindTexture(GL_TEXTURE_2D, frontDepthMap);
glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

glBindFramebuffer(GL_FRAMEBUFFER, hemicubeFramebuffer);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, frontDepthMap, 0);
glDrawBuffer(GL_NONE);
glReadBuffer(GL_NONE);

glViewport(0, 0, resolution, resolution);

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
```

For the vertex program we simply transform the vertices to the clip-space of the patch (i.e. what we could call light-space):

```GLSL
#version 450 core

layout (location = 0) in vec3 vertexPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(vertexPos, 1.0);
}
```

The fragment shader is a simple passthrough shader:

```GLSL
#version 450 core

void main() {

}
```

Afterwards render the other faces (by creating a separate texture for each and setting the proper view matrix for each) and add all of the resulting textures into an `std::vector`. Once done, simply return this `std::vector`.

That's it, the visibility calculation is done! Well...kind of, however, the rest of it is either done in the `updateLightmaps()` function or at least requires knowledge about that function. I'll post two whole versions of the hemicube functions (one long and easy to understand, the other much shorter but not as straightforward - both do the same thing just in different ways). The reason why I'm not including them here is that our current hemicube generation function has one rather hard to spot but still significant bug. You know what your homework is - don't spend too long on it but try to have a guess or two about what our issue might be. I promise to tell you in the next tutorial!