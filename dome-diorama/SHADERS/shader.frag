#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(constant_id = 0) const int SHADING_MODE = 0;

layout(binding = 0, set = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 eyePos;
} ubo;

struct LightData {
    mat4 lightSpaceMatrix;
    vec3 position;
    int type;
    vec3 direction;
    float intensity;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
    int castsShadows;
    int shadowMapIndex;
    float padding;
};

layout(binding = 1, set = 0) uniform LightBuffer {
    LightData lights[8];
    int numLights;
    int numShadowMaps;
    float padding[2];
} lightBuffer;

layout(binding = 0, set = 1) uniform MaterialProperties {
    vec4 albedoColor;
    float roughness;
    float metallic;
    float emissiveIntensity;
    float opacity;
    float indexOfRefraction;
    float heightScale;
    float textureScale;
    float padding2;
} material;

layout(binding = 1, set = 1) uniform sampler2D albedoMap;
layout(binding = 2, set = 1) uniform sampler2D normalMap;
layout(binding = 3, set = 1) uniform sampler2D roughnessMap;
layout(binding = 4, set = 1) uniform sampler2D metallicMap;
layout(binding = 5, set = 1) uniform sampler2D emissiveMap;
layout(binding = 6, set = 1) uniform sampler2D heightMap;
layout(binding = 7, set = 1) uniform sampler2D aoMap;

layout(binding = 0, set = 2) uniform sampler2DShadow shadowMaps[4];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;
layout(location = 4) in vec3 fragLighting;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

float sampleShadowMap(int index, vec3 shadowCoord) {
    switch (index) {
        case 0: return texture(shadowMaps[0], shadowCoord);
        case 1: return texture(shadowMaps[1], shadowCoord);
        case 2: return texture(shadowMaps[2], shadowCoord);
        case 3: return texture(shadowMaps[3], shadowCoord);
        default: return 1.0;
    }
}

vec2 getShadowMapTexelSize(int index) {
    switch (index) {
        case 0: return 1.0 / textureSize(shadowMaps[0], 0);
        case 1: return 1.0 / textureSize(shadowMaps[1], 0);
        case 2: return 1.0 / textureSize(shadowMaps[2], 0);
        case 3: return 1.0 / textureSize(shadowMaps[3], 0);
        default: return vec2(1.0 / 4096.0);
    }
}

float calculateShadow(int lightIndex, vec3 fragPos, vec3 normal, vec3 lightDir) {
    if (lightBuffer.lights[lightIndex].castsShadows == 0) {
        return 1.0;
    }
    
    int shadowMapIndex = lightBuffer.lights[lightIndex].shadowMapIndex;
    if (shadowMapIndex < 0 || shadowMapIndex >= 4) {
        return 1.0;
    }
    
    vec4 fragPosLightSpace = lightBuffer.lights[lightIndex].lightSpaceMatrix * vec4(fragPos, 1.0);
    
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    float currentDepth = projCoords.z - bias;
    
    float shadow = 0.0;
    vec2 texelSize = getShadowMapTexelSize(shadowMapIndex);
    
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            shadow += sampleShadowMap(shadowMapIndex, vec3(projCoords.xy + offset, currentDepth));
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

vec3 calculatePointLight(LightData light, int lightIndex, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float roughness, float metallic) {
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), (1.0 - roughness) * 128.0);
    
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(halfwayDir, viewDir), 0.0), 5.0);
    
    vec3 kS = fresnel;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    vec3 diffuse = kD * albedo / PI;
    vec3 specular = kS * spec;
    
    float shadow = calculateShadow(lightIndex, fragPos, normal, lightDir);
    
    return (diffuse + specular) * light.color * light.intensity * diff * attenuation * shadow;
}

vec3 calculateSunLight(LightData light, int lightIndex, vec3 normal, vec3 viewDir, vec3 albedo, float roughness, float metallic) {
    vec3 lightDir = normalize(-light.direction);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), (1.0 - roughness) * 128.0);
    
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(halfwayDir, viewDir), 0.0), 5.0);
    
    vec3 kS = fresnel;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    vec3 diffuse = kD * albedo / PI;
    vec3 specular = kS * spec;
    
    float shadow = calculateShadow(lightIndex, fragWorldPos, normal, lightDir);
    
    return (diffuse + specular) * light.color * light.intensity * diff * shadow;
}

void main() {
    vec2 scaledTexCoord = fragTexCoord * material.textureScale;
    
    vec4 albedoSample = texture(albedoMap, scaledTexCoord) * material.albedoColor;
    vec3 albedo = albedoSample.rgb;
    float alpha = albedoSample.a * material.opacity;
    
    if (SHADING_MODE == 0) {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(ubo.eyePos - fragWorldPos);
    
    float roughness = material.roughness;
    float metallic = material.metallic;
    
    vec3 ambient = vec3(0.03) * albedo;
    vec3 lighting = ambient;
    
    for (int i = 0; i < lightBuffer.numLights && i < 8; i++) {
        LightData light = lightBuffer.lights[i];
        
        if (light.type == 0) {
            lighting += calculatePointLight(light, i, normal, fragWorldPos, viewDir, albedo, roughness, metallic);
        } else if (light.type == 1) {
            lighting += calculateSunLight(light, i, normal, viewDir, albedo, roughness, metallic);
        }
    }
    
    outColor = vec4(lighting, alpha);
} else {
    outColor = vec4(fragLighting * albedo, alpha);
}
}