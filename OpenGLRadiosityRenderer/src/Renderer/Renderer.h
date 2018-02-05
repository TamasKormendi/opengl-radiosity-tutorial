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

	unsigned int createVisibilityTexture(ObjectModel& model, ShaderLoader& visibilityShader, glm::mat4& mainObjectModelMatrix, glm::mat4& viewMatrix, unsigned int& resolution);

	std::vector<unsigned int> createHemicubeTextures(ObjectModel& model,
										ShaderLoader& hemicubeShader,
										glm::mat4& mainObjectModelMatrix,
										std::vector<glm::mat4>& viewMatrices,
										unsigned int& resolution,
										glm::vec3& shooterWorldspacePos,
										glm::vec3& shooterWorldspaceNormal);

	void updateLightmaps(ObjectModel& model, ShaderLoader& lightmapUpdateShader, glm::mat4& mainObjectModelMatrix, std::vector<glm::mat4>& viewMatrices, std::vector<unsigned int>& visibilityTextures);

	void displayFramebufferTexture(ShaderLoader& debugShader, unsigned int& debugVAO, unsigned int textureID);
};

#endif
