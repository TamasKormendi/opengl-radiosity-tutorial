//This shader program is not used in the main execution anymore - only left here for archival purposes

#version 450 core

layout (location = 0) in vec3 vertexPos;
layout (location = 3) in vec3 inID;

out vec3 fragPos;
out vec3 ID;

uniform mat4 model;
uniform mat4 view;


void main() {
    float far = 100.0;
    float near = 0.1;

    fragPos = vec3(model * vec4(vertexPos, 1.0));

    ID = inID;

    //Hemispherical projection code from https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter39.html

    vec4 modelViewPosition = view * model * vec4(vertexPos, 1.0);

    vec3 hemispherePoint = normalize(modelViewPosition.xyz);

    float farMinusNearPlane = far - near;

    gl_Position.xy = hemispherePoint.xy * farMinusNearPlane;

    gl_Position.z = (-2.0 * modelViewPosition.z - far - near);

    gl_Position.w = farMinusNearPlane;
}