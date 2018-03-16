#version 450 core

layout (location = 0) out vec3 irradianceData;
layout (location = 1) out vec3 radianceData;

uniform sampler2DMS multisampledNewIrradianceTexture;
uniform sampler2DMS multisampledNewRadianceTexture;

void main() {
    vec3 finalIrradiance = vec3(0.0, 0.0, 0.0);
    vec3 finalRadiance = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i <8; ++i) {
        vec3 fetchedIrradiance = texelFetch(multisampledNewIrradianceTexture, ivec2(gl_FragCoord.xy), i).rgb;
        vec3 fetchedRadiance = texelFetch(multisampledNewRadianceTexture, ivec2(gl_FragCoord.xy), i).rgb;

        //Need to initialise them this way so their min value is accurate
        if (i == 0) {
            finalIrradiance = fetchedIrradiance;
            finalRadiance = fetchedRadiance;
        }

        finalIrradiance = max(finalIrradiance, fetchedIrradiance);
        finalRadiance = max(finalRadiance, fetchedRadiance);
    }

    //finalColour /= 8;

    irradianceData = finalIrradiance;
    radianceData = finalRadiance;

}