#version 450

layout(binding = 0, set = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 eyePos;
} ubo;

struct LightData {
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
    mat4 lightSpaceMatrix;
};

layout(binding = 1, set = 0) uniform LightBuffer {
    LightData lights[8];
    int numLights;
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

layout(binding = 0, set = 2) uniform sampler2D shadowMaps[8];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

float calculateShadow(int lightIndex, vec3 fragPos, vec3 normal, vec3 lightDir) {
    if (lightBuffer.lights[lightIndex].castsShadows == 0) {
        return 1.0;
    }
    
    int shadowMapIndex = lightBuffer.lights[lightIndex].shadowMapIndex;
    if (shadowMapIndex < 0) {
        return 1.0;
    }
    
    vec4 fragPosLightSpace = lightBuffer.lights[lightIndex].lightSpaceMatrix * vec4(fragPos, 1.0);
    
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    
    float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
    float bias = 0.0005 * tan(acos(cosTheta));
    bias = clamp(bias, 0.0, 0.001);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMaps[shadowMapIndex], 0);
    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(shadowMaps[shadowMapIndex], projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow /= 25.0;
    
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

vec3 calculateDirectionalLight(LightData light, int lightIndex, vec3 normal, vec3 viewDir, vec3 albedo, float roughness, float metallic) {
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

vec3 calculateSpotLight(LightData light, int lightIndex, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float roughness, float metallic) {
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float spotIntensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    if (theta > light.outerCutOff) {
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
        
        return (diffuse + specular) * light.color * light.intensity * diff * attenuation * spotIntensity * shadow;
    }
    
    return vec3(0.0);
}

void main() {
    vec2 scaledTexCoord = fragTexCoord * material.textureScale;
    
    vec4 albedoSample = texture(albedoMap, scaledTexCoord) * material.albedoColor;
    vec3 albedo = albedoSample.rgb;
    float alpha = albedoSample.a * material.opacity;
    
    if (alpha < 0.5) {
        discard;
    }
    
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
            lighting += calculateDirectionalLight(light, i, normal, viewDir, albedo, roughness, metallic);
        } else if (light.type == 2) {
            lighting += calculateSpotLight(light, i, normal, fragWorldPos, viewDir, albedo, roughness, metallic);
        }
    }
    
    outColor = vec4(lighting, alpha);
}