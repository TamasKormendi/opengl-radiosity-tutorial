//This shader program is not used in the main execution anymore - only left here for archival purposes

#version 450 core

layout (location = 0) out vec3 primitiveID;

in vec3 fragPos;
in vec3 ID;

void main() {

    primitiveID = ID;

}