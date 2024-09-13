#version 450


#if _VERTEX_SHADER
#pragma shader_stage(vertex)

layout(location = 0) out vec2 outUVs;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() 
{
	outUVs = vec2(gl_VertexIndex & 1, gl_VertexIndex >> 1);
    outUVs.y = 1.0f - outUVs.y;
	gl_Position = vec4(outUVs * 2.0f - 1.0f, 0.0f, 1.0f);
}
#endif


#if _FRAGMENT_SHADER
#pragma shader_stage(fragment)


layout(set = 0, binding = 0) uniform sampler2DMS colorSampler;
layout(set = 0, binding = 1) uniform sampler2DMS normalSampler;
layout(set = 0, binding = 2) uniform sampler2DMS specGlossSampler;

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 outColor;

layout (constant_id = 0) const int NUM_SAMPLES = 8;

// Manual resolve for MSAA samples 
vec4 resolve(sampler2DMS _tex, vec2 _UV)
{
    ivec2 texSize = textureSize(_tex);
	ivec2 iUVs = ivec2(_UV * texSize);

	vec4 result = vec4(0.0);	   
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		vec4 val = texelFetch(_tex, iUVs, i); 
		result += val;
	}    
	// Average resolved samples
	return result / float(NUM_SAMPLES);
}

void main() 
{
    vec2 splitScreenUV = inUVs * 2.0f;
    vec2 textureUV = mod(inUVs * 2.0f, 1.0f);
    if(splitScreenUV.x<1.0f && splitScreenUV.y<1.0f)
    {
        outColor = resolve(colorSampler, textureUV); 
    }
    else if(splitScreenUV.x>1.0f && splitScreenUV.y<1.0f)
    {
        outColor = resolve(normalSampler, textureUV); 
    }
    else if(splitScreenUV.x<1.0f && splitScreenUV.y>1.0f)
    {
        outColor = resolve(specGlossSampler, textureUV); 
    }
    else
    {
        // computePBR
        outColor = vec4(1,1,1,1);
    }
}
#endif
