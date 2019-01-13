//Note: when modifying the lightmap update shaders make sure to update the corresponding multisampled ones as well

#version 450 core

layout (location = 0) out vec3 newIrradianceValue;
layout (location = 1) out vec3 newRadianceValue;

in vec3 fragPos;
in vec3 normal;
in vec2 textureCoord;
in vec3 ID;

in vec3 cameraspace_position;
in vec3 normalLightSpace;

in vec4 fragPosLightSpace;

in vec4 fragPosLeftLightSpace;
in vec4 fragPosRightLightSpace;
in vec4 fragPosUpLightSpace;
in vec4 fragPosDownLightSpace;

uniform vec3 shooterRadiance;
uniform vec3 shooterWorldspacePos;
uniform vec3 shooterWorldspaceNormal;
uniform vec2 shooterUV;

uniform sampler2D irradianceTexture;
uniform sampler2D radianceTexture;

uniform sampler2D visibilityTexture;

uniform sampler2D leftVisibilityTexture;
uniform sampler2D rightVisibilityTexture;
uniform sampler2D upVisibilityTexture;
uniform sampler2D downVisibilityTexture;

uniform sampler2D texture_diffuse0;

uniform bool isLamp;

uniform float texelArea;
uniform int attenuationType;

//Parts of this function adapted from https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mappings
float isVisible(sampler2D hemicubeFaceVisibilityTexture, vec4 hemicubeFaceSpaceFragPos) {
    //Add a bit of bias to eliminate shadow acne
    float bias = 0.001;

    //Perform the perspective divide and scale to texture coordinate range
    vec3 projectedCoordinates = hemicubeFaceSpaceFragPos.xyz / hemicubeFaceSpaceFragPos.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;

    float closestDepth = texture(hemicubeFaceVisibilityTexture, projectedCoordinates.xy).r;
    float currentDepth = projectedCoordinates.z;

    float shadow = 0.0;

    //Use percentage-closer filtering (PCF), 25 samples
    vec2 texelSize = 1.0 / textureSize(hemicubeFaceVisibilityTexture, 0);
    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(hemicubeFaceVisibilityTexture, projectedCoordinates.xy + vec2(x, y) * texelSize).r; 
            shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;        
        }    
    }

    shadow = shadow / 25.0;

    float zeroDepth = currentDepth - bias;

    //If the current fragment is in one plane or behind the shooter, it is in shadow
    if (zeroDepth <= 0) {
        //Zero means it is in shadow
        shadow = 0.0;
    }    

    return shadow;
}


void main() {
    float isFragmentVisible = 0.0;

    //Check how visible the fragment is from each hemicube face
    float centreShadow = isVisible(visibilityTexture, fragPosLightSpace);
    float leftShadow = isVisible(leftVisibilityTexture, fragPosLeftLightSpace);
    float rightShadow = isVisible(rightVisibilityTexture, fragPosRightLightSpace);
    float upShadow = isVisible(upVisibilityTexture, fragPosUpLightSpace);
    float downShadow = isVisible(downVisibilityTexture, fragPosDownLightSpace);

    isFragmentVisible = centreShadow + leftShadow + rightShadow + upShadow + downShadow;


    const float pi = 3.1415926535;

    //Form factor between shooter and receiver
    float Fij = 0.0;

    //Adapted from http://sirkan.iit.bme.hu/~szirmay/gpugi1.pdf
    //(Previous versions of this part also used https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter39.html
    //but by now only a few variable names might remain)

    //Calculations are done in camera-space position, if the hemicube front is considered the camera

    //A vector from the shooter to the receiver
    vec3 shooterReceiverVector = normalize(cameraspace_position);
    //This might not be necessary - just in case the normal vectors aren't normalised
    vec3 normalisedNormalLightSpace = normalize(normalLightSpace);

    float distanceSquared = dot(cameraspace_position, cameraspace_position);
    float distanceLinear = length(cameraspace_position);

    float cosThetaX = dot(normalisedNormalLightSpace, -shooterReceiverVector);
    if (cosThetaX < 0) {
        cosThetaX = 0;
    }

    //In camera-space the shooter looks along negative Z
    vec3 shooterDirectionVector = vec3(0, 0, -1);

    float cosThetaY = dot(shooterDirectionVector, shooterReceiverVector);
    if (cosThetaY < 0) {
        cosThetaY = 0;
    } 

    //This conditional avoids division by 0
    if (distanceSquared > 0) {
        float geometricTerm = 0.0;

        if (attenuationType == 0) {
            geometricTerm = cosThetaX * cosThetaY / distanceLinear;
        }
        else if (attenuationType == 1) {
            geometricTerm = cosThetaX * cosThetaY / distanceSquared;
        }
        else if (attenuationType == 2) {
            geometricTerm = (cosThetaX * cosThetaY / (distanceSquared * pi)) * texelArea;
        }

        Fij = geometricTerm;
    }
    else {
        Fij = 0;
    }

    //So far Fij only holds the geometric term, need to multiply by the visibility term
    Fij = Fij * isFragmentVisible;

    vec3 oldIrradianceValue = texture(irradianceTexture, textureCoord).rgb;
    vec3 oldRadianceValue = texture(radianceTexture, textureCoord).rgb;

    vec3 diffuseValue = texture(texture_diffuse0, textureCoord).rgb;

    vec3 deltaIrradiance = Fij * shooterRadiance;
    vec3 deltaRadiance = deltaIrradiance * diffuseValue;

    //Lamps only shoot but do not accumulate - needed to eliminate a back-and-forth selection issue between lamp and a close wall
    if (isLamp) {
        newIrradianceValue = oldIrradianceValue;
        newRadianceValue = oldRadianceValue;
    }
    else {
        newIrradianceValue = oldIrradianceValue + deltaIrradiance;
        newRadianceValue = oldRadianceValue + deltaRadiance;
    }
}