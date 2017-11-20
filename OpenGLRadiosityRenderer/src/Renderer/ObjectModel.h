#ifndef OBJECTMODEL_H
#define OBJECTMODEL_H

//Don't call this function before ilInit() or you're not going going to have a good time
unsigned int loadTexture(const char* path, const std::string& directory);

class ObjectModel {
public:

	ObjectModel(const std::string& path);

	void draw(ShaderLoader& shaderLoader);

private:

	std::vector<Texture> texturesLoaded;
	std::vector<ObjectMesh> meshes;
	std::string directory;

	void loadModel(const std::string& path);

	void processNode(aiNode* node, const aiScene* scene);
	ObjectMesh processMesh(aiMesh* mesh, const aiScene* scene);

	std::vector<Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName);
};

#endif
