#version 450 core

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTextureCoord;
layout (location = 3) in vec3 inID;

out vec3 fragPos;
out vec3 normal;
out vec2 textureCoord;
out vec3 ID;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 ortho;

void main() {
    float far = 100.0;
    float near = 0.1;

    fragPos = vec3(model * vec4(vertexPos, 1.0));

    normal = mat3(transpose(inverse(model))) * inNormal;

    textureCoord = inTextureCoord;

    ID = inID;

    gl_Position = projection * view * vec4(fragPos, 1.0);

    //gl_Position = ortho * view * vec4(fragPos, 1.0);

    //vec2 textSpace = (inTextureCoord - 0.5) * 2;

    //gl_Position = vec4(textSpace.x, textSpace.y, 0.5, 1.0);

    //Hemispherical projection code from https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter39.html

    /*
    vec4 mpos = view * model * vec4(vertexPos, 1.0);

    vec3 hemi_pt = normalize(mpos.xyz);

    float f_minus_n = far - near;

    gl_Position.xy = hemi_pt.xy * f_minus_n;

    gl_Position.z = (-2.0 * mpos.z - far - near);

    gl_Position.w = f_minus_n;
    */

    
}