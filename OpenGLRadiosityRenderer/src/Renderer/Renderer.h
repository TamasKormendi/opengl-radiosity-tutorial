#ifndef RENDERER_H
#define RENDERER_H

class Renderer {
public:
	Renderer();

	void startRenderer();

private:
	void processInput(GLFWwindow* window);
};

#endif
