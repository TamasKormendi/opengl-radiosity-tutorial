#ifndef RENDERER_H
#define RENDERER_H

#include <OpenGLGlobalHeader.h>


class Renderer {
public:
	Renderer();

	void startRenderer(std::string objectFilepath);

private:
	void processInput(GLFWwindow* window);
};

#endif
