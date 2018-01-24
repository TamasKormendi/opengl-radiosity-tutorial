#version 450 core

layout (location = 0) out vec3 primitiveID;

in vec3 fragPos;
in vec3 ID;

void main() {

    primitiveID = ID;

}