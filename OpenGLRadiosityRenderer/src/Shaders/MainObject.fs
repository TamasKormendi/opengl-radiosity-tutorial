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
in vec3 ID;

uniform vec3 viewPos;
uniform Material material;

uniform int lightAmount;
uniform PointLight pointLights[64];

uniform sampler2D texture_diffuse0;
uniform sampler2D texture_specular0;

uniform int addAmbient;

/*
void main() {    
    fragColour = texture(texture_diffuse0, textureCoord);
}
*/

vec3 calculatePointLight(PointLight light, vec3 normalVec, vec3 fragmentPos, vec3 viewDir);


void main() {
    //For now I just don't add the ambient term to the end result, see the "result" calculation
    vec4 alphaTest = texture(texture_diffuse0, textureCoord);
    vec3 specular = texture(texture_specular0, textureCoord).rgb;

    if (alphaTest.a < 0.3) {
        discard;
    }

    vec3 normalisedNormal = normalize(normal);
    vec3 viewDirection = normalize(viewPos - fragPos);

    vec3 result = vec3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < lightAmount; ++i) {
        result += calculatePointLight(pointLights[i], normalisedNormal, fragPos, viewDirection);
    }

    vec3 ambient = vec3(0.2, 0.2, 0.2) * texture(texture_diffuse0, textureCoord).rgb;

    if (addAmbient == 1) {
        result += ambient;
    }

    //Diffuse colour clamping, alphaTest is just the diffuse colour
    /*
    if (result.x > alphaTest.x) {
        result.x = alphaTest.x;
    }
    if (result.y > alphaTest.y) {
        result.y = alphaTest.y;
    }
    if (result.z > alphaTest.z) {
        result.z = alphaTest.z;
    }
    */

    fragColour = vec4(result, 1.0);
}

vec3 calculatePointLight(PointLight light, vec3 normalVec, vec3 fragmentPos, vec3 viewDir) {
    vec3 ambient = light.ambient * texture(texture_diffuse0, textureCoord).rgb;

    vec3 lightDirection = normalize(light.position - fragPos);

    float diffuseAngle = max(dot(normalVec, lightDirection), 0.0);
    vec3 diffuse = light.diffuse * diffuseAngle * texture(texture_diffuse0, textureCoord).rgb;

    vec3 reflectionDirection = reflect(-lightDirection, normalVec);
    float specularAngle = pow(max(dot(viewDir, reflectionDirection), 0.0), 1);
    vec3 specular =  light.specular * specularAngle * texture(texture_specular0, textureCoord).rgb;

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    //float attenuation = 1.0;

    
    ambient *= attenuation;
    diffuse *=attenuation;
    specular *= attenuation;
    

    vec3 result =   //ambient + 
                    diffuse; 
                    //specular;

    return result;
}
