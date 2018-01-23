#ifndef OBJECTMESH_H
#define OBJECTMESH_H

#include <OpenGLGlobalHeader.h>

#include <string>
#include <vector>
#include <iostream>


#include <assimp/Importer.hpp>

#include <Renderer\ShaderLoader.h>

#include <Renderer\Vertex.h>
#include <Renderer\Triangle.h>


struct Texture {
	unsigned int ID;
	std::string type;

	aiString path;
};

class ObjectMesh {
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	std::vector<Triangle> triangles;
	std::vector<Vertex> unwrappedVertices;

	std::vector<GLfloat> irradianceData;
	std::vector<GLfloat> radianceData;

	std::vector<GLfloat> worldspacePosData;
	std::vector<GLfloat> worldspaceNormalData;
	std::vector<GLfloat> idData;
	std::vector<GLfloat> uvData;

	bool isLamp;

	ObjectMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<Texture>& textures, bool isLamp);
	void draw(ShaderLoader& shaderLoader);

private:
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;

	void setupMesh();
};
#endif
