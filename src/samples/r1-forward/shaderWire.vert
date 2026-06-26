#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec3 fragColor;
layout(binding = 0) uniform ObjectData {
    mat4 mvp;
} objectData;


void main() {
    vec4 pos = objectData.mvp * vec4(position, 1.0);
    gl_Position = vec4(pos.x, -pos.y, pos.z, pos.w);
    fragColor = color;
}