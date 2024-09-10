#version 450


#if _VERTEX_SHADER
#pragma shader_stage(vertex)

layout(location = 0) out vec2 outUVs;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() 
{
	outUVs = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUVs * 2.0f - 1.0f, 0.0f, 1.0f);
}
#endif


#if _FRAGMENT_SHADER
#pragma shader_stage(fragment)


layout(set = 0, binding = 1) uniform sampler2DMS colorSampler;
layout(set = 0, binding = 2) uniform sampler2DMS normalSampler;
layout(set = 0, binding = 3) uniform sampler2DMS specGlossSampler;

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = texelFetch(colorSampler, ivec2(inUVs), 0); 
    //outColor = resolve(colorSampler, inUVs);
}
#endif
