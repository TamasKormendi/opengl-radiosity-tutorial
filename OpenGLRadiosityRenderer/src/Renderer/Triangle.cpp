#include "stdafx.h"

#include <OpenGLGlobalHeader.h>

#include <Renderer\Triangle.h>

//We start the ID from 0 so no completely black triangles can exist
int Triangle::integerID = 1;

Triangle::Triangle(Vertex v1, Vertex v2, Vertex v3) {
	vertex1 = v1;
	vertex2 = v2;
	vertex3 = v3;

	calculateRGBID();
}


//This function creates RGB IDs in the range of 1 to 256*256*256 and transforms the IDs to vec3 floats
//TODO: write unit test
void Triangle::calculateRGBID() {

	//This part "overflows" each colour:
	//So if R is on 255 for the previous Triangle then for the current one it would
	//be set to 0 and G would be set to 1 and so on
	float redValue = integerID % 256;

	int greenRemainingValue = (int)integerID / 256;

	float greenValue = greenRemainingValue % 256;

	int blueRemainingValue = (int)greenRemainingValue / 256;

	float blueValue = blueRemainingValue % 256;

	redValue = redValue / 255.0f;
	greenValue = greenRemainingValue / 255.0f;
	blueValue = blueValue / 255.0f;

	glm::vec3 idVector = glm::vec3(redValue, greenValue, blueValue);

	vertex1.rgbID = idVector;
	vertex2.rgbID = idVector;
	vertex3.rgbID = idVector;

	integerID += 1;
}

