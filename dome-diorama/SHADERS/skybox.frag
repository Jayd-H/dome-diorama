#version 450
layout(set = 1, binding = 0) uniform samplerCube skyboxSampler;
layout(location = 0) in vec3 fragTexCoord;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragViewPos;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 viewDir = normalize(fragViewPos - fragWorldPos);
    
    if (dot(viewDir, normalize(fragTexCoord)) > 0.0) {
        discard;
    }
    
    vec3 texCoord = normalize(fragTexCoord);
    texCoord.y = abs(texCoord.y);
    
    outColor = texture(skyboxSampler, texCoord);
}