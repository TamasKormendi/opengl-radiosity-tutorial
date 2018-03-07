#version 450 core

layout (location = 0) out vec3 shooterMeshID;

in vec2 TexCoords;

uniform vec3 meshID;
uniform sampler2D mipmappedRadianceTexture;

void main() {
    //Values for testing output
    int mipmapLevel = textureQueryLevels(mipmappedRadianceTexture);

    vec3 averageEnergyVector = textureLod(mipmappedRadianceTexture, TexCoords, mipmapLevel).rgb;

    float luminance = 0.21 * averageEnergyVector.r + 0.39 * averageEnergyVector.g + 0.4 * averageEnergyVector.b;

    gl_FragDepth = 1 / (1 + luminance);

    shooterMeshID = meshID;
}