#version 450

#extension GL_EXT_shader_texture_lod: enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
// this makes the structures declared with a scalar layout match the c structures
#extension GL_EXT_scalar_block_layout : enable

precision highp float;

#define USE_PUNCTUAL

//--------------------------------------------------------------------------------------
//  PS Inputs
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.glsl"
layout (location = 0) in VS2PS Input;

//--------------------------------------------------------------------------------------
// PS Outputs
//--------------------------------------------------------------------------------------

#ifdef HAS_FORWARD_RT
    layout (location = HAS_FORWARD_RT) out vec4 Output_finalColor;
#endif

#ifdef HAS_SPECULAR_ROUGHNESS_RT
    layout (location = HAS_SPECULAR_ROUGHNESS_RT) out vec4 Output_specularRoughness;
#endif

#ifdef HAS_DIFFUSE_RT
    layout (location = HAS_DIFFUSE_RT) out vec4 Output_diffuseColor;
#endif

#ifdef HAS_NORMALS_RT
    layout (location = HAS_NORMALS_RT) out vec4 Output_normal;
#endif

//--------------------------------------------------------------------------------------
//
// Constant Buffers 
//
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Per Frame structure, must match the one in GlTFCommon.h
//--------------------------------------------------------------------------------------

#include "perFrameStruct.h"

layout (scalar, set=0, binding = 0) uniform perFrame 
{
	PerFrame myPerFrame;
};

//--------------------------------------------------------------------------------------
// PerFrame structure, must match the one in GltfPbrPass.h
//--------------------------------------------------------------------------------------

#include "PixelParams.glsl"

layout (scalar, set=0, binding = 1) uniform perObject 
{
    mat4 myPerObject_u_mCurrWorld;
    mat4 myPerObject_u_mPrevWorld;

	PBRFactors u_pbrParams;
};


//--------------------------------------------------------------------------------------
// mainPS
//--------------------------------------------------------------------------------------

#include "functions.glsl"
#include "shadowFiltering.h"
#include "GLTFPBRLighting.h"

void main()
{
    discardPixelIfAlphaCutOff(Input);

    float alpha;
    float perceptualRoughness;
    vec3 diffuseColor;
    vec3 specularColor;
	getPBRParams(Input, u_pbrParams, diffuseColor, specularColor, perceptualRoughness, alpha);

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;

#ifdef HAS_SPECULAR_ROUGHNESS_RT
    Output_specularRoughness = vec4(specularColor, alphaRoughness);
#endif

#ifdef HAS_DIFFUSE_RT
    Output_diffuseColor = vec4(diffuseColor, 0);
#endif

#ifdef HAS_NORMALS_RT
    Output_normal = vec4((getPixelNormal(Input) + 1) / 2, 0);
#endif

#ifdef HAS_FORWARD_RT
	Output_finalColor = getBaseColor(Input);
#endif
}
