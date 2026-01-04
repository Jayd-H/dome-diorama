#version 450

layout(constant_id = 0) const int SHADING_MODE = 0;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 eyePos;
} ubo;

struct LightData {
    mat4 lightSpaceMatrix;
    vec4 position_intensity;
    vec4 direction_constant;
    vec4 color_linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
    int type;
    int castsShadows;
    int shadowMapIndex;
    vec2 padding;
};

layout(binding = 1, set = 0) uniform LightBuffer {
    LightData lights[8];
    int numLights;
    int numShadowMaps;
    vec2 padding;
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

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPos;
layout(location = 4) out vec3 fragLighting;

const float PI = 3.14159265359;

vec3 calculatePointLight(LightData light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float roughness, float metallic) {
    vec3 position = light.position_intensity.xyz;
    float intensity = light.position_intensity.w;
    float constant = light.direction_constant.w;
    float linear = light.color_linear.w;
    vec3 color = light.color_linear.xyz;

    vec3 lightDir = normalize(position - fragPos);
    float distance = length(position - fragPos);
    float attenuation = 1.0 / (constant + linear * distance + light.quadratic * (distance * distance));
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), (1.0 - roughness) * 128.0);
    
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(halfwayDir, viewDir), 0.0), 5.0);
    
    vec3 kS = fresnel;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    vec3 diffuse = kD * albedo / PI;
    vec3 specular = kS * spec;
    
    return (diffuse + specular) * color * intensity * diff * attenuation;
}

vec3 calculateSunLight(LightData light, vec3 normal, vec3 viewDir, vec3 albedo, float roughness, float metallic) {
    vec3 direction = light.direction_constant.xyz;
    float intensity = light.position_intensity.w;
    vec3 color = light.color_linear.xyz;

    vec3 lightDir = normalize(-direction);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), (1.0 - roughness) * 128.0);
    
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(halfwayDir, viewDir), 0.0), 5.0);
    
    vec3 kS = fresnel;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    vec3 diffuse = kD * albedo / PI;
    vec3 specular = kS * spec;
    
    return (diffuse + specular) * color * intensity * diff;
}

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal;
    
    if (SHADING_MODE == 1) {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(ubo.eyePos - fragWorldPos);
    
    vec3 albedo = inColor;
    float roughness = material.roughness;
    float metallic = material.metallic;
    
    vec3 ambient = vec3(0.03) * albedo;
    vec3 lighting = ambient;
    
    for (int i = 0; i < lightBuffer.numLights && i < 8; i++) {
        LightData light = lightBuffer.lights[i];
        
        if (light.type == 0) {
            lighting += calculatePointLight(light, normal, fragWorldPos, viewDir, albedo, roughness, metallic);
        } else if (light.type == 1) {
            lighting += calculateSunLight(light, normal, viewDir, albedo, roughness, metallic);
        }
    }
    
    fragLighting = lighting;
}
}