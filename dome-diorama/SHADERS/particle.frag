#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float fragLifeRatio;

layout(set = 2, binding = 0) uniform ParticleParams {
    vec3 emitterPosition;
    float time;
    vec3 baseColor;
    float particleLifetime;
    vec3 tipColor;
    float maxParticles;
    vec3 gravity;
    float spawnRadius;
    vec3 initialVelocity;
    float particleScale;
    float fadeInDuration;
    float fadeOutDuration;
    int billboardMode;
    int colorMode;
    float velocityRandomness;
    float scaleOverLifetime;
    float rotationSpeed;
    float padding;
} params;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 coord = fragTexCoord * 2.0 - 1.0;
    float dist = length(coord);
    
    if (dist > 1.0) {
        discard;
    }
    
    float alpha = 1.0 - smoothstep(0.8, 1.0, dist);
    
    float fadeIn = min(1.0, fragLifeRatio / params.fadeInDuration);
    float fadeOut = min(1.0, (1.0 - fragLifeRatio) / params.fadeOutDuration);
    alpha *= fadeIn * fadeOut;
    
    outColor = vec4(fragColor, alpha);
}