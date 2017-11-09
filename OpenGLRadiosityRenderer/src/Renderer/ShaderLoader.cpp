//Adapted from https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader_s.h

#include "stdafx.h"

#include <OpenGLGlobalHeader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <Renderer\ShaderLoader.h>

ShaderLoader::ShaderLoader(const char* initialVertexPath, const char* initialFragmentPath) {
	loadAndCompileShaders(initialVertexPath, initialFragmentPath);
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
}

void ShaderLoader::useProgram() {
	glUseProgram(ID);
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

