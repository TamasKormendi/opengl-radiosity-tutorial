#version 450 core

out vec4 fragColour;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
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

uniform vec3 viewPos;
uniform Material material;
uniform Light light;



void main() {
    //Always should be 0
    vec3 ambient = light.ambient * material.diffuse;

    vec3 normalisedNormal = normalize(normal);
    vec3 lightDirection = normalize(light.position - fragPos);

    float diffuseAngle = max(dot(normalisedNormal, lightDirection), 0.0);
    vec3 diffuse = light.diffuse * diffuseAngle * material.diffuse;

    vec3 viewDirection = normalize(viewPos - fragPos);
    vec3 reflectionDirection = reflect(-lightDirection, normalisedNormal);
    float specularAngle = pow(max(dot(viewDirection, reflectionDirection), 0.0), material.shininess);
    vec3 specular =  light.specular * specularAngle * material.specular;

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    ambient *= attenuation;
    diffuse *=attenuation;
    specular *= attenuation;

    vec3 result =   //ambient + 
                    diffuse + 
                    specular;

    fragColour = vec4(result, 1.0);
}