#version 450

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
    vec3 lightPos;
};

layout(set = 1, binding = 0) uniform UniformBuffer {
   mat4 modelMatrix;
   vec4 color;
};

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 lightDirection;
layout(location = 2) in vec3 vertexPosition;

layout(location = 0) out vec4 outColor;

uint Rand(uint state) {
    uint x = state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    
    return x;
}

float Rand01(uint state)
{
    return Rand(state) / 4294967295.0;
}

void main() {
    vec3 c = color.rgb;
    if(c == vec3(0,0,0))
        c = vec3(1,1,1);

    float alpha = color.a;
    if(alpha == 0)
        alpha = 1;


    uint x = uint(vertexPosition.x * 200 + 425778);
    uint y = uint(vertexPosition.y * 200 + 126753);
    uint z = uint(vertexPosition.z * 200 + 868235);

    uint state = Rand(Rand(Rand(x) + Rand(x)) + Rand(Rand(y + 21341))) + Rand(z + Rand(z));
    float r = Rand01(state);
    if(r > alpha)
    {
        discard;
    }

        
    outColor = vec4(c * max(dot(lightDirection,normal) + 1,0.1),1);
}                      