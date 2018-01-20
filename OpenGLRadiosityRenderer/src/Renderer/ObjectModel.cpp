//Adapted from https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/model.h and the corresponding tutorial

#include "stdafx.h"

#include <OpenGLGlobalHeader.h>

#include <string>
#include <vector>
#include <iostream>

#include <stb_image.h>

#include <Renderer\ObjectModel.h>

unsigned int loadTexture(const char* path, const std::string& directory) {

	stbi_set_flip_vertically_on_load(true);

	std::string fullFilepath = directory + '/' + std::string(path);

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width;
	int height;
	int numberOfComponents;
	unsigned char *data = stbi_load(fullFilepath.c_str(), &width, &height, &numberOfComponents, 0);
	if (data) {
		GLenum format;
		if (numberOfComponents == 1) {
			format = GL_RED;
		}
		else if (numberOfComponents == 3) {
			format = GL_RGB;

		}
		else if (numberOfComponents == 4) {
			format = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		std::cout << "Texture loaded" << std::endl;

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}



ObjectModel::ObjectModel(const std::string& path, bool isLamp) {
	this->isLamp = isLamp;

	loadModel(path);

	//We are not actually going to need this code, most likely;
	/*
	unsigned int vertexOffset = 0;

	for (ObjectMesh mesh : meshes) {
		allSceneVertices.insert(std::end(allSceneVertices), std::begin(mesh.vertices), std::end(mesh.vertices));

		//allSceneIndices.insert(std::end(allSceneIndices), std::begin(mesh.indices), std::end(mesh.indices));

		//We need to add the vertexOffset to indices because the indices start from 0 for every mesh
		//So if we want to merge all of the meshes into one VAO/VBO/EBO then the indices have to be
		//manipulated this way
		for (unsigned int index : mesh.indices) {
			allSceneIndices.push_back(index + vertexOffset);
		}
		
		vertexOffset += mesh.vertices.size();
	}

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, allSceneVertices.size() * sizeof(Vertex), &allSceneVertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, allSceneIndices.size() * sizeof(unsigned int), &allSceneIndices[0], GL_STATIC_DRAW);

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

	glBindVertexArray(0);
	*/
}

void ObjectModel::loadModel(const std::string& path) {
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "ASSIMP ERROR: " << importer.GetErrorString() << std::endl;
		return;
	}

	directory = path.substr(0, path.find_last_of('/'));

	processNode(scene->mRootNode, scene);
}

void ObjectModel::draw(ShaderLoader& shaderLoader) {
	for (int i = 0; i < meshes.size(); ++i) {
		meshes[i].draw(shaderLoader);
	}


	//We are unlikely to need this since we are back to per-mesh drawing
	/*
	unsigned int offset = 0;


	glBindVertexArray(VAO);

	for (ObjectMesh mesh : meshes) {

		mesh.draw(shaderLoader);

		glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, (GLvoid*)(sizeof(unsigned int) * offset));

		glActiveTexture(GL_TEXTURE0);

		offset += mesh.indices.size();
		//++offset;

	}

	glDrawElements(GL_TRIANGLES, allSceneIndices.size(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	*/
}


void ObjectModel::processNode(aiNode* node, const aiScene* scene) {
	for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

		meshes.push_back(processMesh(mesh, scene));
	}

	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		processNode(node->mChildren[i], scene);
	}
}

ObjectMesh ObjectModel::processMesh(aiMesh* mesh, const aiScene* scene) {

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		Vertex vertex;
		glm::vec3 vector;

		//Vertex data
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;

		vertex.position = vector;

		//Normal data
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;

		vertex.normal = vector;

		//Tangent data
		vector.x = mesh->mTangents[i].x;
		vector.y = mesh->mTangents[i].y;
		vector.z = mesh->mTangents[i].z;

		vertex.tangent = vector;

		//Bitangent data
		vector.x = mesh->mBitangents[i].x;
		vector.y = mesh->mBitangents[i].y;
		vector.z = mesh->mBitangents[i].z;

		vertex.bitangent = vector;

		//Texture coordinate data
		if (mesh->mTextureCoords[0]) {
			glm::vec2 textureCoords;

			textureCoords.x = mesh->mTextureCoords[0][i].x;
			textureCoords.y = mesh->mTextureCoords[0][i].y;

			vertex.textureCoords = textureCoords;
		}
		else {
			vertex.textureCoords = glm::vec2(0.0f, 0.0f);
		}

		vertices.push_back(vertex);
	}


	//TODO: This part is probably going to be relevant when starting to implement Radiosity patches
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		//Shouldn't this be a pointer?
		aiFace face = mesh->mFaces[i];

		for (unsigned int j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}

	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	//TODO: I have some concerns about this part of the code so do investigate if something fishy is going on

	std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
	std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");

	//??
	std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
	std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");

	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

	return ObjectMesh(vertices, indices, textures, isLamp);
}


std::vector<Texture> ObjectModel::loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName) {
	std::vector<Texture> textures;

	for (unsigned int i = 0; i < material->GetTextureCount(type); ++i) {
		aiString currentTexturePath;
		material->GetTexture(type, i, &currentTexturePath);

		bool textureAlreadyLoaded = false;

		for (unsigned int j = 0; j < texturesLoaded.size(); ++j) {
			if (std::strcmp(texturesLoaded[j].path.C_Str(), currentTexturePath.C_Str()) == 0) {
				textures.push_back(texturesLoaded[j]);

				textureAlreadyLoaded = true;
				break;
			}
		}
		if (!textureAlreadyLoaded) {
			Texture texture;
			texture.ID = loadTexture(currentTexturePath.C_Str(), this->directory);
			texture.type = typeName;
			texture.path = currentTexturePath;
			textures.push_back(texture);
			texturesLoaded.push_back(texture);
		}

	}

	return textures;
}

//Used for adding lamp objects
void ObjectModel::addMesh(ObjectMesh mesh) {
	meshes.push_back(mesh);
}