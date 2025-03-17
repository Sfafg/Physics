#version 450

struct Material
{
    float r;
    float g;
    float b;
};

layout(std430, binding = 4) buffer MaterialData{
    Material material[];
};

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) flat in vec3 cameraPosition;
layout(location = 4) flat in uint materialID;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 skyColor = vec3(0.4, 0.95, 1.0) * 0.5;
    vec3 lightPosition = vec3(100000,100000,-100000);
    vec3 lightDir = normalize(lightPosition-fragPosition);
    vec3 lightReflection = reflect(-lightDir,normal);
    vec3 cameraDirection = normalize(fragPosition - cameraPosition);    

    vec3 ambientLight = skyColor;
    vec3 diffuseLight = vec3(dot(-lightDir,normal));
    float specular = 0.3 * pow(max(dot(lightReflection, cameraDirection),0),8);
    float fresnelValue = 0.02 + (1 - 0.02) * pow(max(0,1-dot(-cameraDirection,normal)),5);
    vec3 fresnel = fresnelValue * vec3(0.4,0.4,0.4);

    vec3 objectColor = vec3(
        material[materialID].r,
        material[materialID].g,
        material[materialID].b
    );

    vec3 light = vec3(max(ambientLight, diffuseLight)) * objectColor;
    outColor = vec4((light + specular) + fresnel,1);
} 