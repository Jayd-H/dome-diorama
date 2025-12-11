#version 450

layout(binding = 0, set = 1) uniform MaterialProperties {
    vec4 albedoColor;
    float roughness;
    float metallic;
    float emissiveIntensity;
    float opacity;
    float indexOfRefraction;
    float heightScale;
    float padding1;
    float padding2;
} material;

layout(binding = 1, set = 1) uniform sampler2D albedoMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragInstanceColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 albedoSample = texture(albedoMap, fragTexCoord);
    
    vec4 finalColor = albedoSample * material.albedoColor * fragInstanceColor;
    
    float distFromCenter = length(fragTexCoord - vec2(0.5));
    float radialFade = 1.0 - smoothstep(0.0, 0.5, distFromCenter);
    
    finalColor.a *= radialFade;
    
    outColor = finalColor;
}