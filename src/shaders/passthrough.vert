#version 450

layout(std140, binding = 0) uniform CameraMatrices{
    mat4 projection;
    mat4 view;
    mat4 viewProjection;
};

layout(std140, binding = 1) buffer InstanceData{
    mat4 model[];
};

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in uint aObjectID;

void main() {
    gl_Position = viewProjection * model[aObjectID] * vec4(aPosition, 1.0);
}