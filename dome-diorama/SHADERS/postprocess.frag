#version 450

layout(binding = 0) uniform sampler2D screenTexture;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

const float GAMMA = 1.0;
const float EXPOSURE = 1.0;
const float SATURATION = 0.9;
const float CONTRAST = 1.0;
const float VIGNETTE_STRENGTH = 0.3;
const float VIGNETTE_EXTENT = 0.6;
const float CHROMATIC_ABERRATION = 0.0;

vec3 tonemap_reinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 tonemap_aces(vec3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 adjustSaturation(vec3 color, float saturation) {
    float luminance = dot(color, vec3(0.299, 0.587, 0.114));
    return mix(vec3(luminance), color, saturation);
}

vec3 adjustContrast(vec3 color, float contrast) {
    return (color - 0.5) * contrast + 0.5;
}

float vignette(vec2 uv) {
    uv *= 1.0 - uv.yx;
    float vig = uv.x * uv.y * 15.0;
    return pow(vig, VIGNETTE_EXTENT);
}

vec3 chromaticAberration(sampler2D tex, vec2 uv, float amount) {
    vec2 direction = uv - vec2(0.5);
    
    float r = texture(tex, uv - direction * amount).r;
    float g = texture(tex, uv).g;
    float b = texture(tex, uv + direction * amount).b;
    
    return vec3(r, g, b);
}

vec3 sharpen(sampler2D tex, vec2 uv) {
    vec2 texelSize = 1.0 / textureSize(tex, 0);
    
    vec3 center = texture(tex, uv).rgb;
    vec3 top = texture(tex, uv + vec2(0.0, texelSize.y)).rgb;
    vec3 bottom = texture(tex, uv - vec2(0.0, texelSize.y)).rgb;
    vec3 left = texture(tex, uv - vec2(texelSize.x, 0.0)).rgb;
    vec3 right = texture(tex, uv + vec2(texelSize.x, 0.0)).rgb;
    
    vec3 edge = -top - bottom - left - right + center * 5.0;
    return center + edge * 0.3;
}

void main() {
    vec3 color = chromaticAberration(screenTexture, fragTexCoord, CHROMATIC_ABERRATION);
    
    color = sharpen(screenTexture, fragTexCoord);
    
    color *= EXPOSURE;
    
    color = tonemap_aces(color);
    
    color = pow(color, vec3(1.0 / GAMMA));
    
    color = adjustSaturation(color, SATURATION);
    color = adjustContrast(color, CONTRAST);
    
    float vig = vignette(fragTexCoord);
    color *= mix(1.0 - VIGNETTE_STRENGTH, 1.0, vig);
    
    outColor = vec4(color, 1.0);
}