#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float fragLifeRatio;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 coord = fragTexCoord * 2.0 - 1.0;
    float dist = length(coord);
    
    float alpha = 1.0 - smoothstep(0.0, 1.0, dist);
    
    alpha *= (1.0 - fragLifeRatio);
    
    outColor = vec4(fragColor, alpha);
}