#version 450 core

layout (location = 0) out vec3 shooterMeshID;

in vec2 TexCoords;

uniform vec3 meshID;

void main() {
    //Values for testing output
    shooterMeshID = meshID;
}