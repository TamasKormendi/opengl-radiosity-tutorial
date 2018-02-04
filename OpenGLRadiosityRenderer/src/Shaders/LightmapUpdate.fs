#version 450 core

layout (location = 0) out vec3 newIrradianceValue;
layout (location = 1) out vec3 newRadianceValue;

in vec3 fragPos;
in vec3 normal;
in vec2 textureCoord;
in vec3 ID;

in vec3 cameraspace_position;

in vec4 fragPosLightSpace;

uniform vec3 shooterRadiance;
uniform vec3 shooterWorldspacePos;
uniform vec3 shooterWorldspaceNormal;
uniform vec2 shooterUV;

uniform sampler2D irradianceTexture;
uniform sampler2D radianceTexture;
uniform sampler2D visibilityTexture;

uniform sampler2D texture_diffuse0;

/*
int isVisible() {
    vec3 projectedPos = normalize(cameraspace_position);

    projectedPos.xy = projectedPos.xy * 0.5 + 0.5;

    vec3 projectedID = texture(visibilityTexture, projectedPos.xy).rgb;

    float visibilityR = abs(projectedID.r - ID.r);
    float visibilityG = abs (projectedID.g - ID.g);
    float visibilityB = abs (projectedID.b - ID.b);

    //We might need to give this a bound (eg +- 0.001) if we don't get correct values
    if (visibilityR <= 0.01 && visibilityG <= 0.01 && visibilityB <= 0.01 ) {
        return int(1);
    }
    else {
        return int(0);
    }
}
*/


//Parts of this function adapted from https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mappings
float isVisible() {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(visibilityTexture, projCoords.xy).r; 

    float currentDepth = projCoords.z;

    float depthDiff = currentDepth - closestDepth;

    //The bias introduces some peter panning but eliminates the "barcode" artifacts
    float bias = 0.001;

    float shadow = (currentDepth - bias) > closestDepth  ? 0.0 : 1.0;

    float zeroDepth = currentDepth - 0.001;

    if (zeroDepth <= 0) {
        return 0;
    }  

    return int(shadow);
}


void main() {
    vec3 r = shooterWorldspacePos - fragPos;

    //Distance is halved for now
    r = r / 2;

    float distanceSquared = dot(r, r);
    r = normalize(r);

    float cosi = dot(normal, r);
    float cosj = -dot(shooterWorldspaceNormal, r);

    const float pi = 3.1415926535;

    float Fij = 0.0;

    //This if avoids division by 0
    if (distanceSquared > 0) {
        //Removed pi for now
        Fij = max(cosi * cosj, 0) / (distanceSquared);
    }
    else {
        Fij = 0;
    }

    Fij = Fij * isVisible();

    vec3 oldIrradianceValue = texture(irradianceTexture, textureCoord).rgb;
    vec3 oldRadianceValue = texture(radianceTexture, textureCoord).rgb;

    vec3 diffuseValue = texture(texture_diffuse0, textureCoord).rgb;

    vec3 deltaIrradiance = Fij * shooterRadiance;
    vec3 deltaRadiance = deltaIrradiance * diffuseValue;

    //Clamping the values and zeroing out the shooter is missing for now
    //Update: this is how values are clamped for now, shooter zeroing is done in the shooter selection function

    //newIrradianceValue = vec3(isVisible(), isVisible(), isVisible());

    newIrradianceValue = oldIrradianceValue + deltaIrradiance;


    //Instead of this, normalising the value if any exceeds 1 might be more sensible    
    if (newIrradianceValue.r > 1) {
        newIrradianceValue.r = 1;
    }
    if (newIrradianceValue.g > 1) {
        newIrradianceValue.g = 1;
    }
    if (newIrradianceValue.b > 1) {
        newIrradianceValue.b = 1;
    }
    

    newRadianceValue = oldRadianceValue + deltaRadiance;


    
    if (newRadianceValue.r > diffuseValue.r) {
        newRadianceValue.r = diffuseValue.r;
    }
    if (newRadianceValue.g > diffuseValue.g) {
        newRadianceValue.g = diffuseValue.g;
    }
    if (newRadianceValue.b > diffuseValue.b) {
        newRadianceValue.b = diffuseValue.b;
    }
    

}