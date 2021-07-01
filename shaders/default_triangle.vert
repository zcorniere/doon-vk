#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 translation;
    mat4 rotation;
    mat4 scale;
} ubo;

layout (push_constant) uniform constants {
    vec4 position;
	mat4 viewproj;
} cameraData;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTextCoords;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTextCoords;

void main() {
    gl_Position = cameraData.viewproj * (ubo.translation * ubo.rotation  * ubo.scale) * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTextCoords = inTextCoords;
}
