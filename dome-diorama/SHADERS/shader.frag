#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D roughnessMap;
layout(set = 1, binding = 3) uniform sampler2D metallicMap;
layout(set = 1, binding = 4) uniform sampler2D emissiveMap;
layout(set = 1, binding = 5) uniform sampler2D heightMap;
layout(set = 1, binding = 6) uniform sampler2D aoMap;

void main() {
    vec4 albedo = texture(albedoMap, fragTexCoord);
    
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 ambient = vec3(0.3);
    vec3 lighting = ambient + diff * vec3(1.0);
    
    outColor = vec4(albedo.rgb * lighting, albedo.a);
}