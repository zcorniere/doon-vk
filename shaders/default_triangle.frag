#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTextCoords;
layout(location = 2) flat in uint textureIndex;

layout (push_constant) uniform constants {
    vec4 position;
	mat4 viewproj;
} cameraData;

layout(set = 1, binding = 0) uniform sampler2D texSampler[];

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler[textureIndex], fragTextCoords);
}
