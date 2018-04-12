#version 450 core

layout (location = 0) out vec3 shooterMeshID;

in vec2 textureCoord;

uniform vec3 meshID;
uniform sampler2D mipmappedRadianceTexture;

void main() {
    //Check how many mipmap levels there are
    int mipmapLevel = textureQueryLevels(mipmappedRadianceTexture);

    //Retrieve value from the top level
    vec3 averageEnergyVector = textureLod(mipmappedRadianceTexture, textureCoord, mipmapLevel).rgb;

    //Taken from http://sirkan.iit.bme.hu/~szirmay/gpugi1.pdf - at the time of writing (31/03/2018) the document is offline
    //See: https://web.archive.org/web/20170829133851/http://sirkan.iit.bme.hu/~szirmay/gpugi1.pdf
    float luminance = 0.21 * averageEnergyVector.r + 0.39 * averageEnergyVector.g + 0.4 * averageEnergyVector.b;

    //Map the luminance to [0...1] where towards 0 the luminance is higher
    gl_FragDepth = 1 / (1 + luminance);

    shooterMeshID = meshID;
}