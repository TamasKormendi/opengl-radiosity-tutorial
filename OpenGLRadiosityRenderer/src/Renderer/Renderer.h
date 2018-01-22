#ifndef RENDERER_H
#define RENDERER_H

#include <OpenGLGlobalHeader.h>

#include <Renderer\ObjectModel.h>


class Renderer {
public:
	Renderer();

	void startRenderer(std::string objectFilepath);

private:
	void processInput(GLFWwindow* window);

	void preprocess(ObjectModel& model, ShaderLoader& shader, glm::mat4& mainObjectModelMatrix);
};

#endif
