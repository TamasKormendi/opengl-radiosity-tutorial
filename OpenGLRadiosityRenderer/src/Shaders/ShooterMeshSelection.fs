#version 450 core

layout (location = 0) out vec3 shooterMeshID;

in vec2 TexCoords;

void main() {
    //Values for testing output
    shooterMeshID = vec3(0.67, 0.000001, 3.2);
}