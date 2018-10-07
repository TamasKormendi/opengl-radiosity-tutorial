#include "stdafx.h"

#include <OpenGLGlobalHeader.h>

#include <Renderer\Triangle.h>

#include <iostream>

//We start the ID from 1 so no completely black triangles can exist
int Triangle::integerID = 1;

Triangle::Triangle(Vertex v1, Vertex v2, Vertex v3) {
	vertex1 = v1;
	vertex2 = v2;
	vertex3 = v3;

	calculateRGBID();
	area = calculateArea();
}


//This function creates RGB IDs in the range of 1 to 256*256*256 and transforms the IDs to vec3 floats
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
	greenValue = greenValue / 255.0f;
	blueValue = blueValue / 255.0f;

	glm::vec3 idVector = glm::vec3(redValue, greenValue, blueValue);

	vertex1.rgbID = idVector;
	vertex2.rgbID = idVector;
	vertex3.rgbID = idVector;

	//Switched the per-triangle difference to 10 to avoid precision issues
	integerID += 10;
}


//Adapted from https://www.opengl.org/discussion_boards/showthread.php/159771-How-can-I-find-the-area-of-a-3D-triangle
//More explanation here: https://math.stackexchange.com/questions/128991/how-to-calculate-area-of-3d-triangle
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

