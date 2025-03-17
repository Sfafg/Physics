#version 450

struct RenderObject
{
    uint objectID;
    uint batchID;
    uint meshID;
};

layout(std140, binding = 0) uniform CameraMatrices{
    mat4 projection;
    mat4 view;
    mat4 viewProjection;
};

layout(std140, binding = 1) buffer InstanceData{
    mat4 model[];
};

layout(std430, binding = 2) buffer RenderObjects {
   RenderObject renderObjects[];
};

layout(std430, binding = 3) buffer BatchData {
   uint batchMaterial[];
};

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in uint aObjectID;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 fragPosition;
layout(location = 3) flat out vec3 cameraPosition;
layout(location = 4) flat out uint materialID;

void main() {
    gl_Position = viewProjection * model[aObjectID] * vec4(aPosition, 1.0);
    normal = normalize((model[aObjectID] * vec4(aNormal,0.0)).xyz);
    uv = aUV;
    fragPosition = (model[aObjectID] * vec4(aPosition,1.0)).xyz;
    cameraPosition = (inverse(view))[3].xyz;
    materialID = batchMaterial[renderObjects[aObjectID].batchID] & 255;
}