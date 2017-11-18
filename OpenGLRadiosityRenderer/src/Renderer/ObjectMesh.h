#ifndef OBJECTMESH_H
#define OBJECTMESH_H

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 textureCoords;

	glm::vec3 tangent;
	glm::vec3 bitangent;
};

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

	ObjectMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<Texture>& textures);
	void draw(ShaderLoader& shaderLoader);

private:
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;

	void setupMesh();
};
#endif
