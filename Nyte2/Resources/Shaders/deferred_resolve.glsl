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

layout(set = 0, binding = 0) uniform UniformBufferObject 
{
   vec4 cameraPosition;
   vec4 lightPosition;
   vec4 lightDirection;
} ubo;
layout(set = 0, binding = 1) uniform sampler2DMS worldPosSampler;
layout(set = 0, binding = 2) uniform sampler2DMS colorSampler;
layout(set = 0, binding = 3) uniform sampler2DMS normalSampler;
layout(set = 0, binding = 4) uniform sampler2DMS specGlossSampler;

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 outColor;

layout (constant_id = 0) const int NUM_SAMPLES = 8; // TODO: use push constant or else to set this value









float PI = 3.141592653589793f;
vec3 WHITE = vec3(1.0f, 1.0f, 1.0f);

float rcp(float f) { return 1.0f / f; }

//// Normal Distribution function --------------------------------------
//float D_GGX(float dotNH, float roughness)
//{
//	float alpha = roughness * roughness;
//	float alpha2 = alpha * alpha;
//	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
//	return (alpha2)/(PI * denom*denom);
//}
//
//// Geometric Shadowing function --------------------------------------
//float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
//{
//	float r = (roughness + 1.0);
//	float k = (r*r) / 8.0;
//	float GL = dotNL / (dotNL * (1.0 - k) + k);
//	float GV = dotNV / (dotNV * (1.0 - k) + k);
//	return GL * GV;
//}
//
//// Fresnel function ----------------------------------------------------
//vec3 F_Schlick(float cosTheta, float metallic, vec3 materialColor)
//{
//	vec3 F0 = mix(vec3(0.04, 0.04, 0.04), materialColor, metallic); // * material.specular
//	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
//	return F;
//}
//
//// Specular BRDF composition --------------------------------------------
//vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness, vec3 materialColor)
//{
//	// Precalculate vectors and dot products
//	vec3 H = normalize (V + L);
//	float dotNV = clamp(dot(N, V), 0.0, 1.0);
//	float dotNL = clamp(dot(N, L), 0.0, 1.0);
//	float dotLH = clamp(dot(L, H), 0.0, 1.0);
//	float dotNH = clamp(dot(N, H), 0.0, 1.0);
//
//	// Light color fixed
//	vec3 lightColor = vec3(1.0, 1.0, 1.0);
//
//	vec3 color = vec3(0.0, 0.0, 0.0);
//
//	if (dotNL > 0.0)
//	{
//		float rroughness = max(0.05, roughness);
//		// D = Normal distribution (Distribution of the microfacets)
//		float D = D_GGX(dotNH, roughness);
//		// G = Geometric shadowing term (Microfacets shadowing)
//		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
//		// F = Fresnel factor (Reflectance depending on angle of incidence)
//		vec3 F = F_Schlick(dotNV, metallic, materialColor);
//
//		vec3 spec = D * F * G / (4.0 * dotNL * dotNV);
//
//		color += spec * dotNL * lightColor;
//	}
//
//	return color;
//}


// Fresnel Reflectance (Schlick Approximation)
// It describes an object's tendency to reflect at glancing angles.
// note: here _half = normalize(_viewDir + _lightDir)
vec3 FresnelReclectance(vec3 _viewDir, vec3 _half, vec3 _specular)
{
	return _specular + (1.0f - _specular) * pow(1.0f - dot(_half, _viewDir), 5.0);
}

// Geometry Term (Smith GGX)
// It describes microfacet shadowing based on roughness and distribution.
float GeometryTerm(float _roughness, float _nDotL, float _nDotV)
{
	float roughnessActual = _roughness * _roughness;
	float viewGeoTerm = _nDotV + sqrt((_nDotV - _nDotV*roughnessActual) * _nDotV + roughnessActual);
	float lightGeoTerm = _nDotL + sqrt((_nDotL - _nDotL*roughnessActual) * _nDotL + roughnessActual);
	return rcp(viewGeoTerm * lightGeoTerm);
}

// Normal Distribution Term (Trowbridge-Reitz)
// It describes the distribution of microfacets across the surface of the object.
float NormalDistributionTerm(float _roughness, vec3 _surfaceNormal, vec3 _microfacetNormal)
{
	float roughnessActual = _roughness * _roughness;
	float roughnessActualSquared = pow(roughnessActual, 2.0);
	return roughnessActualSquared / (
		PI * pow( 
			pow(dot(_surfaceNormal, _microfacetNormal), 2.0) 
			* (roughnessActualSquared - 1.0f) + 1.0f, 2.0)
	);
}

// Specular Term
vec3 SpecularTerm(vec3 _viewDir, vec3 _lightDir, vec3 _normal, vec3 _diffuse, float _metallic, float _roughness, float _specularFactor)
{
	vec3 halfVL = normalize(_viewDir + _lightDir);
	float nDotV = dot(_normal,_viewDir);
	float nDotL = dot(_normal,_lightDir);
	vec3 surfaceSpecularColor = _diffuse; // this could also be given by the material as a specific input

	vec3 surfaceSpecular = mix(WHITE, surfaceSpecularColor, _metallic);
	vec3 specularColor = vec3(_metallic, _metallic, _metallic) * surfaceSpecular * _specularFactor;

	vec3 specularTerm = FresnelReclectance(_viewDir, halfVL, specularColor);
	specularTerm *= GeometryTerm(_roughness, nDotL, nDotV);
	specularTerm *= NormalDistributionTerm(_roughness, _normal, halfVL);
	//specularTerm *= shadowContrib; // no shadows yet
	specularTerm *= nDotL;
	return specularTerm;
}

// Diffuse Color
vec3 Diffuse(vec3 _viewDir, vec3 _lightDir, vec3 _normal, vec3 _diffuse, float _metallic)
{
	float nDotL = dot(_normal,_lightDir);
	
	vec3 diffuse = _diffuse;
	diffuse *= nDotL;
	//diffuse *= shadowContrib; // no shadows yet
	diffuse *= (1.0f - _metallic); // avoid any diffuse color for metallic material
	return diffuse;
}

// Ambient Color
vec3 Ambient(vec3 _ambientLightColor, vec3 _diffuse, float _metallic)
{
	vec3 metallicAmbientColor = vec3(0.2f, 0.2f, 0.2f);

	vec3 ambient = _ambientLightColor;
	ambient *= mix(_diffuse, metallicAmbientColor, _metallic);
	return ambient;
}

vec3 Environment()
{
	vec3 environment = vec3(0.0f, 0.0f, 0.0f);
	return environment;
}


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
    const uint dbgCellCount = 4;
    const float dbgCellWidth = 1.0f / float(dbgCellCount);
    const float dbgCellHeight = 0.2f;

    vec2 textureUV = vec2(mod(inUVs.x * float(dbgCellCount), 1.0f), inUVs.y/dbgCellHeight);

	if(inUVs.x<dbgCellWidth*1.0f && inUVs.y<dbgCellHeight)
    {
        outColor = resolve(worldPosSampler, textureUV);
    }
    else if(inUVs.x<dbgCellWidth*2.0f && inUVs.y<dbgCellHeight)
    {
        outColor = resolve(colorSampler, textureUV);
    }
    else if(inUVs.x<dbgCellWidth*3.0f && inUVs.y<dbgCellHeight)
    {
        outColor = resolve(normalSampler, textureUV); 
    }
    else if(inUVs.x<dbgCellWidth*4.0f && inUVs.y<dbgCellHeight)
    {
        outColor = resolve(specGlossSampler, textureUV); 
    }
    else
    {
		vec3 worldPos = resolve(worldPosSampler, inUVs).xyz; 
		vec4 diffuse = resolve(colorSampler, inUVs);
		vec3 normal = resolve(normalSampler, inUVs).xyz; 
		vec4 specGloss = resolve(specGlossSampler, inUVs); 

		vec3 viewDir = normalize(ubo.cameraPosition.xyz - worldPos);
		vec3 lightDir = normalize(ubo.lightDirection.xyz);
		float metallic = specGloss.r;
		float roughness = specGloss.a;
		float specularFactor = 1.0f;
		vec3 ambientLightColor = vec3(51.0f/255.0f, 51.0f/255.0f, 0.0f);

		vec3 finalColor = Diffuse(viewDir, lightDir, normal, diffuse.rgb, metallic);
		finalColor += SpecularTerm(viewDir, lightDir, normal, diffuse.rgb, metallic, roughness, specularFactor);
		finalColor += Ambient(ambientLightColor, diffuse.rgb, metallic);
		finalColor += Environment();


		outColor = vec4(finalColor, 1.0f);

		// computePBR
//		vec3 V = normalize(ubo.cameraPosition.xyz - worldPos);
//
//		float roughness = 1.0f - specGloss.a;
//
//		// Add striped pattern to roughness based on vertex position
//	#ifdef ROUGHNESS_PATTERN
//		roughness = max(roughness, step(frac(worldPos.y * 2.02), 0.5));
//	#endif
//
//		// Specular contribution
//		vec3 Lo = vec3(0.0, 0.0, 0.0);
//		for (int i = 0; i < 4; i++) {
//			vec3 L = normalize(ubo.lightPosition.xyz - worldPos);
//			Lo += BRDF(L, V, normal, specGloss.r, roughness, diffuse.rgb);
//		};
//
//		// Combine with ambient
//		vec3 color = diffuse.rgb * 0.02f;
//		color += Lo;
//
//		// Gamma correct
//		color = pow(color, vec3(0.4545, 0.4545, 0.4545));
//
//        outColor = vec4(color, 1.0f);

		//outColor = resolve(colorSampler, inUVs);
    }
}
#endif
