#version 450


#if _VERTEX_SHADER
#pragma shader_stage(vertex)

layout(set = 0, binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo_MVP;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTexCoords;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo_MVP.proj * ubo_MVP.view * ubo_MVP.model * vec4(inPosition, 1.0);
    outColor = inColor;
    outTexCoords = inTexCoords;
}
#endif


#if _FRAGMENT_SHADER
#pragma shader_stage(fragment)


layout(set = 0, binding = 1) uniform sampler2D texSamplerLinearRepeat;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoords;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(texSamplerLinearRepeat, inTexCoords);
    color.rgb *= inColor;

    outColor = color;
}
#endif
