#version 450 core

out vec4 fragColour;

uniform vec3 objectColour;
uniform vec3 lightColour;

void main() {
    fragColour = vec4(lightColour * objectColour, 1.0);
}