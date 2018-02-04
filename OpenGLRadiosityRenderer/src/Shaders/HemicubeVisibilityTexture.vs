#version 450 core

layout (location = 0) in vec3 vertexPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

/*
layout (location = 3) in vec3 inID;

out vec3 ID;
*/

void main() {

    //ID = inID;

    gl_Position = projection * view * model * vec4(vertexPos, 1.0);
}