#ifndef OBJECTMODEL_H
#define OBJECTMODEL_H

//TODO: SORT OUT IMPORTS ASAP

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include <Renderer\ShaderLoader.h>

#include <Renderer\ObjectMesh.h>

unsigned int loadTexture(const char* path, const std::string& directory);

class ObjectModel {
public:

	ObjectModel(const std::string& path);

	void draw(ShaderLoader& shaderLoader);

private:

	std::vector<Vertex> allSceneVertices;
	std::vector<unsigned int> allSceneIndices;

	std::vector<Texture> texturesLoaded;
	std::vector<ObjectMesh> meshes;
	std::string directory;

	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;

	void loadModel(const std::string& path);

	void processNode(aiNode* node, const aiScene* scene);
	ObjectMesh processMesh(aiMesh* mesh, const aiScene* scene);

	std::vector<Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName);
};

#endif
