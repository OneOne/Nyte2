#version 450

#if _VERTEX_SHADER
#pragma shader_stage(vertex)

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
#endif


#if _FRAGMENT_SHADER
#pragma shader_stage(fragment)

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
#endif
