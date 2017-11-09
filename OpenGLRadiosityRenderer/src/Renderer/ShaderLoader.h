#ifndef SHADERLOADER_H
#define SHADERLOADER_H

class ShaderLoader {
public:
	unsigned int ID;

	ShaderLoader(const char* initialVertexPath, const char* initialFragmentPath);

	void loadAndCompileShaders(const char* vertexPath, const char* fragmentPath);

	void useProgram();
private:

	void checkCompileErrors(unsigned int shader, std::string type);

};

#endif 
