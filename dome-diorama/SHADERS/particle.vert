#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in float inParticleIndex;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec3 eyePos;
} camera;

layout(set = 2, binding = 0) uniform ParticleParams {
    vec3 emitterPosition;
    float time;
    vec3 baseColor;
    float waveFrequency;
    vec3 tipColor;
    float waveAmplitude;
    vec3 velocityBase;
    float particleLifetime;
    float upwardSpeed;
    float particleScale;
    float spawnRadius;
    float maxParticles;
} params;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out float fragLifeRatio;

const float PI = 3.14159265359;

uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

float random(uint seed) {
    return float(hash(seed)) / 4294967295.0;
}

void main() {
    uint particleId = uint(inParticleIndex);
    
    float particleTime = mod(params.time + (inParticleIndex / params.maxParticles) * params.particleLifetime, params.particleLifetime);
    float lifeRatio = particleTime / params.particleLifetime;
    
    uint seed1 = particleId * 2u + 1u;
    uint seed2 = particleId * 2u + 2u;
    uint seed3 = particleId * 3u + 3u;
    
    float spawnX = (random(seed1) - 0.5) * params.spawnRadius * 2.0;
    float spawnZ = (random(seed2) - 0.5) * params.spawnRadius * 2.0;
    
    vec3 particlePos = params.emitterPosition + vec3(spawnX, 0.0, spawnZ);
    
    float sineWave = sin(particleTime * params.waveFrequency + spawnX * 2.0);
    particlePos.x += sineWave * params.waveAmplitude * lifeRatio;
    
    float velocityX = (random(seed1 + 100u) - 0.5) * 0.5;
    float velocityZ = (random(seed2 + 100u) - 0.5) * 0.5;
    
    particlePos.y += particleTime * params.upwardSpeed;
    particlePos.x += particleTime * velocityX;
    particlePos.z += particleTime * velocityZ;
    
    float scale = params.particleScale * (1.0 - lifeRatio * 0.5);
    
    vec3 toCamera = normalize(camera.eyePos - particlePos);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, toCamera));
    up = cross(toCamera, right);
    
    vec3 billboardPos = particlePos + (right * inPosition.x + up * inPosition.y) * scale;
    
    gl_Position = camera.proj * camera.view * vec4(billboardPos, 1.0);
    
    fragColor = mix(params.baseColor, params.tipColor, lifeRatio);
    fragTexCoord = inTexCoord;
    fragLifeRatio = lifeRatio;
}