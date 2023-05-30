#define CONCAT(a,b) a ## b
#define TEXCOORD(id) CONCAT(Input.UV, id)

//disable texcoords that are not in the VS2PS structure
#if defined(ID_TEXCOORD_0)==false
    #if ID_normalTexCoord == 0
        #undef ID_normalTexture
        #undef ID_normalTexCoord
    #endif
    #if ID_emissiveTexCoord == 0
        #undef ID_emissiveTexture
        #undef ID_emissiveTexCoord
    #endif
    #if ID_baseTexCoord == 0
        #undef ID_baseColorTexture
        #undef ID_baseTexCoord
    #endif
    #if ID_metallicRoughnessTexCoord == 0
        #undef ID_metallicRoughnessTexture
        #undef ID_metallicRoughnessTexCoord
    #endif
#endif

#ifdef ID_baseColorTexture
    layout (set=1, binding = ID_baseColorTexture) uniform sampler2D u_BaseColorSampler;
#endif

#ifdef ID_normalTexture
    layout (set=1, binding = ID_normalTexture) uniform sampler2D u_NormalSampler;
#endif

#ifdef ID_emissiveTexture
    layout (set=1, binding = ID_emissiveTexture) uniform sampler2D u_EmissiveSampler;
#endif

#ifdef ID_metallicRoughnessTexture
    layout (set=1, binding = ID_metallicRoughnessTexture) uniform sampler2D u_MetallicRoughnessSampler;
#endif

#ifdef ID_occlusionTexture
    layout (set=1, binding = ID_occlusionTexture) uniform sampler2D u_OcclusionSampler;
    float u_OcclusionStrength  =1.0;
#endif

#ifdef ID_diffuseTexture
    layout (set=1, binding = ID_diffuseTexture) uniform sampler2D u_diffuseSampler;
#endif

#ifdef ID_specularGlossinessTexture
    layout (set=1, binding = ID_specularGlossinessTexture) uniform sampler2D u_specularGlossinessSampler;
#endif

#ifdef USE_IBL
    layout (set=1, binding = ID_diffuseCube) uniform samplerCube u_DiffuseEnvSampler;
    layout (set=1, binding = ID_specularCube) uniform samplerCube u_SpecularEnvSampler;
#define USE_TEX_LOD
#endif

#ifdef ID_brdfTexture
    layout (set=1, binding = ID_brdfTexture) uniform sampler2D u_brdfLUT;
#endif

//------------------------------------------------------------
// UV getters
//------------------------------------------------------------
vec2 GetNormalUV(VS2PS Input)
{
    vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef ID_normalTexCoord
    uv.xy = TEXCOORD(ID_normalTexCoord);
    #ifdef HAS_NORMAL_UV_TRANSFORM
        uv *= u_NormalUVTransform;
    #endif
#endif
    return uv.xy;
}