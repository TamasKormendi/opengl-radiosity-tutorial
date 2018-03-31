//Adapted from https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/mesh.h Vertex struct

#ifndef VERTEX_H
#define VERTEX_H

#include <OpenGLGlobalHeader.h>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 textureCoords;

	glm::vec3 rgbID;

	glm::vec3 tangent;
	glm::vec3 bitangent;
};

#endif