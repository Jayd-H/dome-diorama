#version 450

layout(binding = 0, set = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 4) in mat4 instanceModel;
layout(location = 8) in vec4 instanceColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragInstanceColor;

void main() {
    vec3 billboardPos = inPosition;
    
    vec3 cameraRight = vec3(ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]);
    vec3 cameraUp = vec3(ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]);
    
    vec3 worldPos = vec3(instanceModel[3][0], instanceModel[3][1], instanceModel[3][2]);
    float scale = length(vec3(instanceModel[0][0], instanceModel[1][0], instanceModel[2][0]));
    
    worldPos += cameraRight * billboardPos.x * scale;
    worldPos += cameraUp * billboardPos.y * scale;
    
    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
    
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragInstanceColor = instanceColor;
}