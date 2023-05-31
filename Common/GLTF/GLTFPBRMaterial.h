#pragma once

#include "Utilities/PCH.h"
#include "GLTFHelpers.h"
#include "Utilities/ShaderCompiler.h"

struct PBRMaterialParametersConstantBuffer
{
    math::Vector4 mEmissiveFactor;
    // GLTF: pbrMetallicRoughness
    math::Vector4 mBaseColorFactor;
    math::Vector4 mMetalicRoughnessFactor;

    // KHR_materials_pbrSpecularGlossiness
    math::Vector4 mDiffuseFactor;
    math::Vector4 mSpecularGlossinessFactor;
};

struct PBRMaterialParameters
{
    bool mDoubleSided = false;
    bool mBlending = false;

    DefineList mDefines;
    PBRMaterialParametersConstantBuffer mParams;
};

//
// Read GLTF material and store it in our structure
//
void SetDefaultMaterialParameters(PBRMaterialParameters* pPBRMaterialParam);
void ProcessMaterials(const json::object_t& material, PBRMaterialParameters* gltfMat, std::map<std::string, int>& textureIdx);
bool DoesMaterialUseSemantic(DefineList &defines, std::string semanticName);
bool ProcessGetTextureIndexAndTextCoord(const json::object_t &material, const std::string &textureName, int *pIndex, int *pTexCoord);
void GetSrgbAndCutOffOfImageGivenItsUse(int imageIndex, const json &materials, bool *pSrgbOut, float *pCutoff);