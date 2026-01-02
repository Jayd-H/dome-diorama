#version 450
layout(set = 1, binding = 0) uniform samplerCube skyboxSampler;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec3 eyePos;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 domeCenter;
    float domeRadiusSquared;
} push;

layout(location = 0) in vec3 fragTexCoord;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragViewPos;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 toDome = camera.eyePos - push.domeCenter;
    float distSquared = dot(toDome, toDome);
    
    if (distSquared >= push.domeRadiusSquared) {
        discard;
    }
    
    vec3 texCoord = normalize(fragTexCoord);
    texCoord.y = abs(texCoord.y);
    
    outColor = texture(skyboxSampler, texCoord);
}