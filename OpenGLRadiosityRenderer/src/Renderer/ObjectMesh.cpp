//Code adapted from https://learnopengl.com/#!Model-Loading/Mesh

#include "stdafx.h"

#include <OpenGLGlobalHeader.h>

#include <string>
#include <vector>
#include <iostream>


#include <assimp/Importer.hpp>

#include <Renderer\ShaderLoader.h>
#include <Renderer\ObjectMesh.h>

#include <Renderer\RadiosityConfig.h>

ObjectMesh::ObjectMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<Texture>& textures, bool isLamp, float scale) {
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	this->isLamp = isLamp;

	this->scale = scale;

	this->overallArea = 0.0;

	if (isLamp) {
		irradianceData = std::vector<GLfloat>(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3, 1.0f);
		radianceData = std::vector<GLfloat>(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3, 6.0f);
	}
	else {
		irradianceData = std::vector<GLfloat>(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3, 0.0f);
		radianceData = std::vector<GLfloat>(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3, 0.0f);
	}

	worldspacePosData = std::vector<GLfloat>(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3, 0.0f);
	worldspaceNormalData = std::vector<GLfloat>(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3, 0.0f);

	idData = std::vector<GLfloat>(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3, 0.0f);
	uvData = std::vector<GLfloat>(::RADIOSITY_TEXTURE_SIZE * ::RADIOSITY_TEXTURE_SIZE * 3, 0.0f);

	texturespaceShooterIndices = std::vector<unsigned int>();

	setupMesh();

	//The final overall area is scale^2 * sum area of triangles obtained in setupMesh()
	overallArea = (scale * scale) * overallArea;

	//This is properly initialised in the preprocess step
	texelArea = 0;
}


void ObjectMesh::draw(ShaderLoader& shaderLoader) {
	unsigned int diffuseNumber = 0;
	unsigned int specularNumber = 0;
	unsigned int normalNumber = 0;
	unsigned int heightNumber = 0;

	
	for (unsigned int i = 0; i < textures.size(); ++i) {
		//We start the texture units from 7 since we leave the first two for the ir/radiance textures and 5 others for the visibility textures
		glActiveTexture(GL_TEXTURE7 + i);

		std::string number;
		std::string name = textures[i].type;

		//Only the diffuse texture is used in the current renderer
		if (name == "texture_diffuse") {
			number = std::to_string(diffuseNumber++);
		}
		else if (name == "texture_specular") {
			number = std::to_string(specularNumber++);
		}
		else if (name == "texture_normal") {
			number = std::to_string(normalNumber++);
		}
		else if (name == "texture_height") {
			number = std::to_string(heightNumber++);
		}

		//See above
		shaderLoader.setUniformInt((name + number).c_str(), i + 7);
		glBindTexture(GL_TEXTURE_2D, textures[i].ID);
	}

	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, unwrappedVertices.size());

	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
}
void ObjectMesh::setupMesh() {

	for (unsigned int i = 0; i < indices.size(); i += 3) {

		Triangle triangle(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]);

		overallArea += triangle.area;

		triangles.push_back(triangle);
	}

	//These can be uncommented to reset (and thus not store) the original vertex and index data if they are not needed
	//vertices = std::vector<Vertex>();
	//indices = std::vector<unsigned int>();

	for (Triangle triangle : triangles) {
		unwrappedVertices.push_back(triangle.vertex1);
		unwrappedVertices.push_back(triangle.vertex2);
		unwrappedVertices.push_back(triangle.vertex3);
	}

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	//glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, unwrappedVertices.size() * sizeof(Vertex), &unwrappedVertices[0], GL_STATIC_DRAW);

	//Vertex position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	//Vertex normal
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	//Vertex texture coordinates
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, textureCoords));

	//Vertex triangle ID
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, rgbID));

	//These ones are not used
	//Vertex tangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

	//Vertex bitangent
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

	glBindVertexArray(0);
}