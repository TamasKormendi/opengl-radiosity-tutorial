//Code based on: https://learnopengl.com/In-Practice/Debugging

#version 450 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 inTextureCoord;

out vec2 textureCoord;

void main() {
    gl_Position = vec4(position, 0.0f, 1.0f);
    textureCoord = inTextureCoord;
}