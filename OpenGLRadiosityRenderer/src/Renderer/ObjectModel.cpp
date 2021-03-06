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



ObjectModel::ObjectModel(const std::string& path, bool isLamp, float scale) {
	this->isLamp = isLamp;

	this->scale = scale;

	loadModel(path);
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

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];

		for (unsigned int j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}

	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
	std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");

	//Only diffuse textures are handled properly by the program but apparently Assimp considers normal maps as "HEIGHT" and height maps as "AMBIENT"
	std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
	std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");

	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

	return ObjectMesh(vertices, indices, textures, isLamp, scale);
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