#ifndef RENDERER_H
#define RENDERER_H

#include <OpenGLGlobalHeader.h>

#include <Renderer\ObjectModel.h>


class Renderer {
public:
	Renderer();

	void setParameters(int& rendererResolution, int& lightmapResolution, int& attenuationType, bool& continuousUpdate, bool& textureFiltering, bool& multisampling);

	void startRenderer(std::string objectFilepath);

private:
	int rendererResolution;
	int lightmapResolution;
	int attenuationType;
	bool continuousUpdate;
	bool textureFiltering;
	bool multisampling;

	void processInput(GLFWwindow* window);

	void preprocess(ObjectModel& model, ShaderLoader& shader, glm::mat4& mainObjectModelMatrix);

	unsigned int selectShooterMesh(ObjectModel& model, ShaderLoader& shooterMeshSelectionShader, unsigned int& screenAlignedQuadVAO);

	void selectMeshBasedShooter(ObjectModel& model, glm::vec3& shooterRadiance, glm::vec3& shooterWorldspacePos, glm::vec3& shooterWorldspaceNormal, glm::vec2& shooterUV, unsigned int& shooterMeshIndex);

	void selectShooter(ObjectModel& model, glm::vec3& shooterRadiance, glm::vec3& shooterWorldspacePos, glm::vec3& shooterWorldspaceNormal, glm::vec2& shooterUV, int& shooterMeshIndex);

	unsigned int createVisibilityTexture(ObjectModel& model, ShaderLoader& visibilityShader, glm::mat4& mainObjectModelMatrix, glm::mat4& viewMatrix, unsigned int& resolution);

	std::vector<unsigned int> createHemicubeTextures(
		ObjectModel& model,
		ShaderLoader& hemicubeShader,
		glm::mat4& mainObjectModelMatrix,
		std::vector<glm::mat4>& viewMatrices,
		unsigned int& resolution,
		glm::vec3& shooterWorldspacePos,
		glm::vec3& shooterWorldspaceNormal
	);

	void updateLightmaps(ObjectModel& model, ShaderLoader& lightmapUpdateShader, glm::mat4& mainObjectModelMatrix, std::vector<glm::mat4>& viewMatrices, std::vector<unsigned int>& visibilityTextures);

	void displayFramebufferTexture(ShaderLoader& debugShader, unsigned int& debugVAO, unsigned int textureID);

	void preprocessMultisample(ObjectModel& model, ShaderLoader& shader, glm::mat4& mainObjectModelMatrix, ShaderLoader& resolveShader, unsigned int& screenAlignedQuadVAO);

	void updateLightmapsMultisample(ObjectModel& model, ShaderLoader& lightmapUpdateShader, glm::mat4& mainObjectModelMatrix, std::vector<glm::mat4>& viewMatrices, std::vector<unsigned int>& visibilityTextures, ShaderLoader& resolveShader, unsigned int& screenAlignedQuadVAO);

};

#endif
