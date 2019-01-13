# Chapter 3 - Triangles and object loader modifications

Welcome back! Today we'll look at what the Triangle class does and we'll also take a brief look at the modifications I made to the oject loader classes. You might've also noticed that, unlike https://learnopengl.com/ I always keep separate header files (with declarations and structs) and .cpp files (for implementation). This is just my preferred stylistic choice, follow whichever you please!

### The Triangle class
As mentioned in the previous chapter this renderer also has a class for triangles, so we can give each of them a unique RGB ID and also calculate their area.

Giving a unique ID to an object is not hard, right? Keep and increment a static counter whenever a class is instantiated. Problem solved! Or is it? Since we want to use the ID during GPU draw operations we can't just keep an integer counter since when we write to a texture it is usually done in RGB format, commonly with values between 0-1 float (inclusive). So we need to be a bit crafty about this. We also want to maximise how many values our system can handle.

Let's not go too crazy and just allow 256 values (0 - 255) per colour channel so we're sure not to encounter any precision issues. In the best case this'd let us use 256^3 == 16777216 values and thus as many individual triangles in our scene. Since our lightmaps and geometry are fairly well decoupled this should serve well enough.

Small note: we don't want completely dark, RGB(0, 0, 0) triangle IDs so we actually have (256^3) - 1 possible values. Sorry if this is incredibly disappointing to anyone. :(

Without further ado, how can we do this? Well, we have to maximise the use of each channel so we can't just blindly increment values. The system I settled on using is a sort of "overflow counter" system, from Red through Green to Blue values. Say, if the Red channel is on 255 (the highest it should store) and we instantiate a new triangle the Red value resets to 0 BUT the Green value increments to 1 and so on and so forth.

I'll show another example but before that, let's look at the code:

Triangle.cpp

```cpp
float redValue = integerID % 256;

int greenRemainingValue = (int)integerID / 256;

float greenValue = greenRemainingValue % 256;

int blueRemainingValue = (int)greenRemainingValue / 256;

float blueValue = blueRemainingValue % 256;
```

The "flip" point is 256 (the value that should never be reached) so we use that both in our modulo operation and our integer division. Let's look at another example while working through the code, line by line. Say, let this example be 10000.

The first line `10000 % 256` would result in `16`, which we know is going to be our Red value since it gives us the value that is going to be present after all the times Red overflowed. Then we can get the next value by seeing how many times Red reset to 0 and thus the next value (Green) incremented. This is going to be the result of an integer division (10000 divided by 256), which is `39`. After this the whole process restarts, just with the `greenValue` and the `greenRemainingValue` variables. In our case `greenValue` would still be `39` while `blueRemainingValue` and `blueValue` would both be zero, since integer division 39 / 256 is 0 and thus 0 % 256 is 0 as well.

This gives us an RGB value of (16, 39, 0). How can we check that this is correct? Well, our current system is sort of like the decimal system, just reversed and with 256 values. In the decimal system, say "200" would mean 2 times 10^2, 0 times 10^1 and 0 times 10^0. In our system, we have 16 times 256^0, 39 times 256^1 and 0 times 256^2. If we sum these up it results in, yep, you guessed it, 10000! I would encourage you to run through a few examples on paper if this one example wasn't quite satisfactory enough.

The rest of the function simply maps the values we got to the 0-1 float range (0 is 0 and 255 is 1, since we can never exceed 255) and assigns this value to each of the vertices of the triangle:

```cpp
redValue = redValue / 255.0f;
greenValue = greenValue / 255.0f;
blueValue = blueValue / 255.0f;

glm::vec3 idVector = glm::vec3(redValue, greenValue, blueValue);

vertex1.rgbID = idVector;
vertex2.rgbID = idVector;
vertex3.rgbID = idVector;
```

I also have this line in the code:
```cpp
integerID += 10;
```

You can safely change this to:

```cpp
++integerID;
```

It should cause no problems.

That leaves the triangle area calculation code. I feel this Maths Stackexchange answer explains it in an incredibly understandable way so I'd recommend you give it a read before taking a look at the code: https://math.stackexchange.com/a/1951650

The code itself is pretty much what you see on the previous webpage, just C++-ified, of course:

```cpp
float Triangle::calculateArea() {
	glm::vec3 v1ToV2;
	glm::vec3 v1ToV3;
	glm::vec3 normalVector;

	float area;

	v1ToV2.x = vertex2.position.x - vertex1.position.x;
	v1ToV2.y = vertex2.position.y - vertex1.position.y;
	v1ToV2.z = vertex2.position.z - vertex1.position.z;

	v1ToV3.x = vertex3.position.x - vertex1.position.x;
	v1ToV3.y = vertex3.position.y - vertex1.position.y;
	v1ToV3.z = vertex3.position.z - vertex1.position.z;

	normalVector.x = v1ToV2.y * v1ToV3.z - v1ToV2.z * v1ToV3.y;
	normalVector.y = v1ToV2.z * v1ToV3.x - v1ToV2.x * v1ToV3.z;
	normalVector.z = v1ToV2.x * v1ToV3.y - v1ToV2.y * v1ToV3.x;

	area = 0.5 * sqrt(normalVector.x * normalVector.x + normalVector.y * normalVector.y + normalVector.z * normalVector.z);

	return area;
}
```

If you want a bit more information about it, this code was partly adapted from https://www.opengl.org/discussion_boards/showthread.php/159771-How-can-I-find-the-area-of-a-3D-triangle so you could take a look there too.

### Object loader modifications

This part is mostly about small and rather simple changes so instead of copy-pasting code blocks here I'd encourage you to take a look at the `ObjectMesh.cpp` and the `ObjectModel.cpp` files and their headers. I'll point out the biggest changes here:

ObjectMesh.cpp:

* The `ObjectMesh` keeps track of the geometric and lightmap data, along with the possible shooter indices.
* The `ObjectMesh` also keeps track of its overall area, texel area, whether it is a lamp or not (if it is a lamp we initialise the lightmap data vectors with non-0 values - HDR is not fully implemented in the current engine, however, radiance values can go above 1.0, so we can initialise a lamp to have, say (6.0, 6.0, 6.0) as its radiance value) and its scale factor - this is important since if we scale a mesh when we transform it to world-space that operation is also going to change its area.
* The textures start from `GL_TEXTURE7` since we're going to be using the textures before that for other purposes.
* The `setupMesh()` function does a few things differently: it creates triangles out of vertices using the index buffer then loads the modified vertex data (with added RGB ID value) to another buffer - we are not using an EBO to render since an ID value is also stored in vertices so two vertices cannot be identical.

ObjectModel.cpp:

* We also keep track of the scale and whether the meshes within the model are lamps or not - mostly to pass these information to the meshes themselves.
* I opted to remove the `aiProcess_FlipUVs` flag when loading a file with AssImp and instead used `stbi_set_flip_vertically_on_load(true);` when loading a texture - this might not make too much of a difference in the long run, however, this was my personal preference.

That's it for today! This was a slightly shorter chapter, mostly to settle a few things that would not fit into other chapters. Having said that I hope you enjoyed it and I'll see you next time with...the first part of our radiosity loop. See you then!