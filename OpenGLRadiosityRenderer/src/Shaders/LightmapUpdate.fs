#version 450 core

layout (location = 0) out vec3 newIrradianceValue;
layout (location = 1) out vec3 newRadianceValue;

in vec3 fragPos;
in vec3 normal;
in vec2 textureCoord;
in vec3 ID;

in vec3 cameraspace_position;

uniform vec3 shooterRadiance;
uniform vec3 shooterWorldspacePos;
uniform vec3 shooterWorldspaceNormal;
uniform vec2 shooterUV;

uniform sampler2D irradianceTexture;
uniform sampler2D radianceTexture;
uniform sampler2D visibilityTexture;

uniform sampler2D texture_diffuse0;

int isVisible() {
    vec3 projectedPos = normalize(cameraspace_position);

    projectedPos.xy = projectedPos.xy * 0.5 + 0.5;

    vec3 projectedID = texture(visibilityTexture, projectedPos.xy).rgb;

    float visibilityR = abs(projectedID.r - ID.r);
    float visibilityG = abs (projectedID.g - ID.g);
    float visibilityB = abs (projectedID.b - ID.b);

    //We might need to give this a bound (eg +- 0.001) if we don't get correct values
    if (visibilityR <= 0.02 && visibilityG <= 0.02 && visibilityB <= 0.02 ) {
        return int(1);
    }
    else {
        return int(0);
    }
}

void main() {
    vec3 r = shooterWorldspacePos - fragPos;
    float distanceSquared = dot(r, r);
    r = normalize(r);

    float cosi = dot(normal, r);
    float cosj = -dot(shooterWorldspaceNormal, r);

    const float pi = 3.1415926535;

    float Fij = max(cosi * cosj, 0) / (pi * distanceSquared);

    Fij = Fij * isVisible();

    vec3 oldIrradianceValue = texture(irradianceTexture, textureCoord).rgb;
    vec3 oldRadianceValue = texture(radianceTexture, textureCoord).rgb;

    vec3 diffuseValue = texture(texture_diffuse0, textureCoord).rgb;

    vec3 deltaIrradiance = Fij * shooterRadiance;
    vec3 deltaRadiance = deltaIrradiance * diffuseValue;

    //Clamping the values and zeroing out the shooter is missing for now

    newIrradianceValue = oldIrradianceValue + deltaIrradiance;
    newRadianceValue = oldRadianceValue + deltaRadiance;
}