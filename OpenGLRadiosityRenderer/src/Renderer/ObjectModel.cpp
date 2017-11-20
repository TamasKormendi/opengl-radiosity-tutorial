//Adapted from https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/model.h and the corresponding tutorial

#include "stdafx.h"

#include <OpenGLGlobalHeader.h>

#include <string>
#include <vector>
#include <iostream>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include <IL\il.h>

#include <Renderer\ShaderLoader.h>
#include <Renderer\ObjectMesh.h>
#include <Renderer\ObjectModel.h>



//Some additional code for this function from:
//http://www.geeks3d.com/20090105/tutorial-how-to-load-and-display-an-image-with-devil-and-opengl/
//and
//http://www.lighthouse3d.com/cg-topics/code-samples/loading-an-image-and-creating-a-texture/
unsigned int loadTexture(const char* path, const std::string& directory) {

	//TODO: Refactor fullFilename because it looks a bit repulsive
	std::string fullFilename = std::string(path);
	
	fullFilename = directory + '/' + fullFilename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	unsigned int imageID;
	ILboolean success;

	ilGenImages(1, &imageID);
	ilBindImage(imageID);


	//TODO: problems might arise here
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

	success = ilLoadImage((ILstring)fullFilename.c_str());

	if (!success) {
		std::cout << "DevIL: Image failed to load. Path tried: " << path << std::endl;

		ilDeleteImages(1, &imageID);
		return 0;
	}
	else {
		GLenum imageFormat;

		switch (ilGetInteger(IL_IMAGE_FORMAT)) {
		case IL_ALPHA:
			imageFormat = GL_RED; ilConvertImage(IL_ALPHA, IL_UNSIGNED_BYTE); break;
		case IL_RGB:
			imageFormat = GL_RGB; ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE); break;
		//The default includes RGBA too
		default:
			imageFormat = GL_RGBA; ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE); break;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, imageFormat, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, imageFormat, GL_UNSIGNED_BYTE, ilGetData());
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		ilDeleteImages(1, &imageID);
	}

	return textureID;
}



ObjectModel::ObjectModel(const std::string& path) {
	loadModel(path);
}

void ObjectModel::draw(ShaderLoader& shaderLoader) {
	for (ObjectMesh mesh : meshes) {
		mesh.draw(shaderLoader);
	}
}

void ObjectModel::loadModel(const std::string& path) {
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "ASSIMP ERROR: " << importer.GetErrorString() << std::endl;
		return;
	}

	directory = path.substr(0, path.find_last_of('/'));

	processNode(scene->mRootNode, scene);
}

void ObjectModel::processNode(aiNode* node, const aiScene* scene) {
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
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

	return ObjectMesh(vertices, indices, textures);
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