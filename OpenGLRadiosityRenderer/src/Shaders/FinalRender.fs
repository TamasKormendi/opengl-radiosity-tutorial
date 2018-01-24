#version 450 core

out vec4 fragColour;

in vec2 textureCoord;

uniform sampler2D irradianceTexture;
//uniform sampler2D radianceTexture;

uniform sampler2D texture_diffuse0;

uniform int addAmbient;

void main() {
    vec3 result = vec3(0.0f, 0.0f, 0.0f);

    vec3 ambient = vec3(0.2, 0.2, 0.2) * texture(texture_diffuse0, textureCoord).rgb;

    if (addAmbient == 1) {
        result += ambient;
    }

    vec3 irradianceValue = texture(irradianceTexture, textureCoord).rgb;

    vec3 diffuseValue = texture(texture_diffuse0, textureCoord).rgb;

    result += irradianceValue * diffuseValue;

    fragColour = vec4(result, 1.0);
}