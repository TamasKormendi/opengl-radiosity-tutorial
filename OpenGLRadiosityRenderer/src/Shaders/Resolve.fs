#version 450 core

layout (location = 0) out vec3 posData;
layout (location = 1) out vec3 normalData;
layout (location = 2) out vec3 idData;
layout (location = 3) out vec3 uvData;

uniform sampler2DMS multisampledPosTexture;
uniform sampler2DMS multisampledNormalTexture;
uniform sampler2DMS multisampledIDTexture;
uniform sampler2DMS multisampledUVTexture;

void main() {
    vec3 finalPos = vec3(0.0, 0.0, 0.0);
    vec3 finalNormal = vec3(0.0, 0.0, 0.0);
    vec3 finalID = vec3(0.0, 0.0, 0.0);
    vec3 finalUV = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i <8; ++i) {
        vec3 fetchedPos = texelFetch(multisampledPosTexture, ivec2(gl_FragCoord.xy), i).rgb;
        vec3 fetchedNormal = texelFetch(multisampledNormalTexture, ivec2(gl_FragCoord.xy), i).rgb;
        vec3 fetchedID = texelFetch(multisampledIDTexture, ivec2(gl_FragCoord.xy), i).rgb;
        vec3 fetchedUV = texelFetch(multisampledUVTexture, ivec2(gl_FragCoord.xy), i).rgb;

        //Need to initialise them this way so their min value is accurate
        if (i == 0) {
            finalPos = fetchedPos;
            finalNormal = fetchedNormal;
            finalID = fetchedID;
            finalUV = fetchedUV;
        }



        finalPos = max(finalPos, fetchedPos);
        finalNormal = max(finalNormal, fetchedNormal);
        finalID = max(finalID, fetchedID);
        finalUV = max(finalUV, fetchedUV);
    }

    //finalColour /= 8;

    posData = finalPos;
    normalData = finalNormal;
    idData = finalID;
    uvData = finalUV;

}