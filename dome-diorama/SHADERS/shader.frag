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
layout(binding = 2, set = 1) uniform sampler2D normalMap;
layout(binding = 3, set = 1) uniform sampler2D roughnessMap;
layout(binding = 4, set = 1) uniform sampler2D metallicMap;
layout(binding = 5, set = 1) uniform sampler2D emissiveMap;
layout(binding = 6, set = 1) uniform sampler2D heightMap;
layout(binding = 7, set = 1) uniform sampler2D aoMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 albedo = texture(albedoMap, fragTexCoord) * material.albedoColor;
    outColor = vec4(albedo.rgb, 1.0);
}