#version 450 core

layout (location = 0) out vec3 worldspacePosition;
layout (location = 1) out vec3 worldspaceNormal;
layout (location = 2) out vec3 triangleID;
layout (location = 3) out vec3 interpolatedUV;

centroid in vec3 fragPos;
centroid in vec3 normal;
centroid in vec2 textureCoord;
centroid in vec3 ID;

void main() {

    worldspacePosition = fragPos;

    //If we ever need non-normalised normals, look here
    worldspaceNormal = normalize(normal);

    triangleID = ID;

    interpolatedUV = vec3(textureCoord, 0.0);

}