#ifndef SHADERLOADER_H
#define SHADERLOADER_H

#include <vector>

class ShaderLoader {
public:
	static std::vector<ShaderLoader*> listOfShaders;

	static void reloadShaders();

	unsigned int ID;

	ShaderLoader(const char* initialVertexPath, const char* initialFragmentPath);

	void loadAndCompileShaders(const char* vertexPath, const char* fragmentPath);

	void useProgram();

	//Uniform setting functions

	void setUniformBool(const std::string &name, bool value) const;

	void setUniformInt(const std::string &name, int value) const;

	void setUniformFloat(const std::string &name, float value) const;

	void setUniformVec2(const std::string &name, const glm::vec2 &value) const;
	void setUniformVec2(const std::string &name, float x, float y) const;

	void setUniformVec3(const std::string &name, const glm::vec3 &value) const;
	void setUniformVec3(const std::string &name, float x, float y, float z) const;

	void setUniformVec4(const std::string &name, const glm::vec4 &value) const;
	void setUniformVec4(const std::string &name, float x, float y, float z, float w);

	void setUniformMat2(const std::string &name, const glm::mat2 &mat) const;

	void setUniformMat3(const std::string &name, const glm::mat3 &mat) const;

	void setUniformMat4(const std::string &name, const glm::mat4 &mat) const;
private:
	std::string storedVertexPath;
	std::string storedFragmentPath;

	void checkCompileErrors(unsigned int shader, std::string type);

};

#endif 
