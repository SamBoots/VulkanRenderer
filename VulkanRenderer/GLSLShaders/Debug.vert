#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( push_constant ) uniform InstanceModel {
    mat4 model;
} model;

layout(set = 0, binding = 0) uniform ViewProjection {
    mat4 view;
    mat4 proj;
} vp;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;

void main() 
{
    gl_Position = vp.proj * vp.view * model.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}