#version 450 core

layout (location = 1) out vec3 normalData;

uniform sampler2DMS multisampledTexture;

void main() {
    vec3 finalColour = vec3(0.0, 0.0, 0.0);
    int samplesHit = 0;

    for (int i = 0; i <8; ++i) {
        vec3 fetchedColours = texelFetch(multisampledTexture, ivec2(gl_FragCoord.xy), i).rgb;

        /*
        if (fetchedColours.r == 0.0 && fetchedColours.g == 0.0 && fetchedColours.b == 0.0) {
            ++samplesHit;
        }
        else {
            ++samplesHit;
        }
        */

        ++samplesHit;

        finalColour = max(finalColour, fetchedColours);
    }

    //finalColour /= 8;

    normalData = finalColour;


}