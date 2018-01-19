//Code adapted from https://learnopengl.com/#!Model-Loading/Mesh

#include "stdafx.h"

#include <OpenGLGlobalHeader.h>

#include <string>
#include <vector>
#include <iostream>


#include <assimp/Importer.hpp>

#include <Renderer\ShaderLoader.h>
#include <Renderer\ObjectMesh.h>

long long int meshAmount = 0;

ObjectMesh::ObjectMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<Texture>& textures) {
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	setupMesh();
}


void ObjectMesh::draw(ShaderLoader& shaderLoader) {
	unsigned int diffuseNumber = 0;
	unsigned int specularNumber = 0;
	unsigned int normalNumber = 0;
	unsigned int heightNumber = 0;

	for (unsigned int i = 0; i < textures.size(); ++i) {
		glActiveTexture(GL_TEXTURE0 + i);

		std::string number;
		std::string name = textures[i].type;

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

		shaderLoader.setUniformInt((name + number).c_str(), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].ID);
	}

	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, unwrappedVertices.size());

	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
}
void ObjectMesh::setupMesh() {
	for (unsigned int i = 0; i < indices.size(); i += 3) {
		Triangle triangle(vertices[i], vertices[i + 1], vertices[i + 2]);

		triangles.push_back(triangle);
	}

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

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, textureCoords));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

	//std::cout << "VAO AMOUNT: " << meshAmount++ << std::endl;

	glBindVertexArray(0);
}