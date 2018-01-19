#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <Renderer\Vertex.h>

/*struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 textureCoords;

	glm::vec3 rgbID;

	glm::vec3 tangent;
	glm::vec3 bitangent;
};*/

class Triangle {
public:
	Vertex vertex1;
	Vertex vertex2;
	Vertex vertex3;

	Triangle(Vertex v1, Vertex v2, Vertex v3);

private:
	static int integerID;

	void calculateRGBID();

};

#endif
