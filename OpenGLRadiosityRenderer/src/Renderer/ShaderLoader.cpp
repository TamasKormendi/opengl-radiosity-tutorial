//Adapted from https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader_s.h and the corresponding tutorial

#include "stdafx.h"

#include <OpenGLGlobalHeader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <Renderer\ShaderLoader.h>

std::vector<ShaderLoader*> ShaderLoader::listOfShaders;

ShaderLoader::ShaderLoader(const char* initialVertexPath, const char* initialFragmentPath) {
	loadAndCompileShaders(initialVertexPath, initialFragmentPath);

	listOfShaders.push_back(this);
}

void ShaderLoader::loadAndCompileShaders(const char* vertexPath, const char* fragmentPath) {
	std::string vertexCode;
	std::string fragmentCode;

	std::ifstream vertexShaderFile;
	std::ifstream fragmentShaderFile;

	vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fragmentShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try {
		vertexShaderFile.open(vertexPath);
		fragmentShaderFile.open(fragmentPath);

		std::stringstream vertexShaderStream;
		std::stringstream fragmentShaderStream;

		vertexShaderStream << vertexShaderFile.rdbuf();
		fragmentShaderStream << fragmentShaderFile.rdbuf();

		vertexShaderFile.close();
		fragmentShaderFile.close();

		vertexCode = vertexShaderStream.str();
		fragmentCode = fragmentShaderStream.str();
	}
	catch (std::ifstream::failure e) {
		std::cout << "Shader files not successfully read" << std::endl;
	}

	//TODO: Check if these lead to leaks and/or can be made smart pointers
	const char* vertexCCode = vertexCode.c_str();
	const char* fragmentCCode = fragmentCode.c_str();

	unsigned int vertexShaderID;
	unsigned int fragmentShaderID;

	vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderID, 1, &vertexCCode, NULL);
	glCompileShader(vertexShaderID);
	checkCompileErrors(vertexShaderID, "VERTEX");


	fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderID, 1, &fragmentCCode, NULL);
	glCompileShader(fragmentShaderID);
	checkCompileErrors(fragmentShaderID, "FRAGMENT");

	ID = glCreateProgram();
	glAttachShader(ID, vertexShaderID);
	glAttachShader(ID, fragmentShaderID);
	glLinkProgram(ID);
	checkCompileErrors(ID, "PROGRAM");

	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	storedVertexPath = std::string(vertexPath);
	storedFragmentPath = std::string(fragmentPath);
}

void ShaderLoader::useProgram() {
	glUseProgram(ID);
}

void ShaderLoader::reloadShaders() {
	for (ShaderLoader* loader : listOfShaders) {
		glDeleteProgram(loader->ID);

		loader->loadAndCompileShaders(loader->storedVertexPath.c_str(), loader->storedFragmentPath.c_str());
	}
}

void ShaderLoader::checkCompileErrors(unsigned int shaderID, std::string type) {
	int success;
	char infoLog[1024];
	if (type != "PROGRAM") {
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shaderID, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n" << std::endl;
		}
	}
	else {
		glGetProgramiv(shaderID, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shaderID, 1024, NULL, infoLog);
			std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n" << std::endl;
		}
	}
}

void ShaderLoader::setUniformBool(const std::string &name, bool value) const {
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void ShaderLoader::setUniformInt(const std::string &name, int value) const {
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void ShaderLoader::setUniformFloat(const std::string &name, float value) const {
	glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void ShaderLoader::setUniformVec2(const std::string &name, const glm::vec2 &value) const {
	glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void ShaderLoader::setUniformVec2(const std::string &name, float x, float y) const {
	glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}

void ShaderLoader::setUniformVec3(const std::string &name, const glm::vec3 &value) const {
	glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void ShaderLoader::setUniformVec3(const std::string &name, float x, float y, float z) const {
	glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void ShaderLoader::setUniformVec4(const std::string &name, const glm::vec4 &value) const {
	glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void ShaderLoader::setUniformVec4(const std::string &name, float x, float y, float z, float w) {
	glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}

void ShaderLoader::setUniformMat2(const std::string &name, const glm::mat2 &mat) const {
	glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void ShaderLoader::setUniformMat3(const std::string &name, const glm::mat3 &mat) const {
	glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void ShaderLoader::setUniformMat4(const std::string &name, const glm::mat4 &mat) const {
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

