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

	void selectShooter(ObjectModel& model, glm::vec3& shooterRadiance, glm::vec3& shooterWorldspacePos, glm::vec3& shooterWorldspaceNormal, glm::vec2& shooterUV, int& shooterMeshIndex);
};

#endif
