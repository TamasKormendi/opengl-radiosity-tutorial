//Code based on https://learnopengl.com/In-Practice/Debugging

#version 450 core
out vec4 FragColor;
in  vec2 TexCoords;
  
uniform sampler2D framebufferTexture;
  
void main() {
    FragColor = texture(framebufferTexture, TexCoords);
} 