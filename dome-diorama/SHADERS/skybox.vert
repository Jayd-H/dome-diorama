#version 450
layout(location = 0) in vec3 inPosition;
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec3 eyePos;
} camera;
layout(location = 0) out vec3 fragTexCoord;
void main() {
    fragTexCoord = normalize(inPosition);
    
    mat4 viewNoTranslation = mat4(mat3(camera.view));
    vec4 pos = camera.proj * viewNoTranslation * vec4(inPosition * 5000.0, 1.0);
    
    gl_Position = pos.xyww;
}