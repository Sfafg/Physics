#version 450

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
    vec3 lightPos;
};

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 lightDirection;

layout(location = 0) out vec4 outColor;

void main() {
    float diffuse = dot(normal,lightDirection);
    diffuse = (diffuse + 1) * 0.5;
    vec3 col = mix(vec3(0.1,0.1,1),vec3(1,0.1,0.1),diffuse);
    outColor = vec4(col,1);
}