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
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoords;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    mat4 mvp = ubo_MVP.proj * ubo_MVP.view * ubo_MVP.model;
    gl_Position = mvp * vec4(inPosition, 1.0);
    outNormal = (mvp * vec4(inNormal, 1.0)).xyz;
    outTexCoords = vec2(inTexCoords.x, 1.0f-inTexCoords.y);
}
#endif


#if _FRAGMENT_SHADER
#pragma shader_stage(fragment)


layout(set = 0, binding = 1) uniform sampler2D diffuseSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D specularSampler;
layout(set = 0, binding = 4) uniform sampler2D glossinessSampler;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoords;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outSpecGloss;

void main() {
    outColor = texture(diffuseSampler, inTexCoords);

    vec3 N = normalize(inNormal);
	vec3 T = normalize(cross(N, vec3(0.0f, 0.0f, 1.0f)));
	vec3 B = normalize(cross(N, T));
    //T = normalize(cross(B, T));
	mat3 TBN = mat3(T, B, N);
	outNormal = vec4(normalize(TBN * texture(normalSampler, inTexCoords).xyz), 1.0f);

    outSpecGloss.rgb = texture(specularSampler, inTexCoords).rbg;
    outSpecGloss.a = texture(glossinessSampler, inTexCoords).a;
}
#endif
