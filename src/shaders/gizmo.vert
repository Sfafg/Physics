#version 450

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
    vec3 lightPosition;
};

layout(set = 1, binding = 0) uniform UniformBuffer {
   mat4 modelMatrix;
   vec4 color;
};

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec3 lightDirection;
layout(location = 2) out vec3 vertexPosition;

void main() {
    vec4 worldPos = modelMatrix * vec4(aPosition, 1.0);
    gl_Position = projection * view * worldPos;
    normal = (modelMatrix * vec4(aNormal,0)).xyz;
    lightDirection =normalize(lightPosition - worldPos.xyz);
    vertexPosition = worldPos.xyz;
}
                 