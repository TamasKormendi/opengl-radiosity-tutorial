#version 450 core

out vec4 fragColour;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct PointLight {
    vec3 position;

    vec3 ambient; //Ambient is likely to get eliminated since it should be handled by radiosity
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

in vec3 fragPos;
in vec3 normal;
in vec2 textureCoord;

uniform vec3 viewPos;
uniform Material material;

uniform int lightAmount;
uniform PointLight pointLights[64];

uniform sampler2D texture_diffuse1;

void main() {    
    fragColour = texture(texture_diffuse1, textureCoord);
}

/*
vec3 calculatePointLight(PointLight light, vec3 normalVec, vec3 fragmentPos, vec3 viewDir);


void main() {
    //For now I just don't add the ambient term to the end result, see the "result" calculation
    vec3 normalisedNormal = normalize(normal);
    vec3 viewDirection = normalize(viewPos - fragPos);

    vec3 result = vec3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < lightAmount; ++i) {
        result += calculatePointLight(pointLights[i], normalisedNormal, fragPos, viewDirection);
    }

    fragColour = vec4(result, 1.0);
}

vec3 calculatePointLight(PointLight light, vec3 normalVec, vec3 fragmentPos, vec3 viewDir) {
    vec3 ambient = light.ambient * material.diffuse;

    vec3 lightDirection = normalize(light.position - fragPos);

    float diffuseAngle = max(dot(normalVec, lightDirection), 0.0);
    vec3 diffuse = light.diffuse * diffuseAngle * material.diffuse;

    vec3 reflectionDirection = reflect(-lightDirection, normalVec);
    float specularAngle = pow(max(dot(viewDir, reflectionDirection), 0.0), material.shininess);
    vec3 specular =  light.specular * specularAngle * material.specular;

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    
    ambient *= attenuation;
    diffuse *=attenuation;
    specular *= attenuation;
    

    vec3 result =   //ambient + 
                    diffuse + 
                    specular;

    return result;
}
*/