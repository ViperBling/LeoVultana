#include "GLTFPbrMaterial.h"
#include "GLTFHelpers.h"

void SetDefaultMaterialParameters(PBRMaterialParameters *pPBRMaterialParam)
{
    pPBRMaterialParam->mDoubleSided = false;
    pPBRMaterialParam->mBlending = false;

    pPBRMaterialParam->mParams.mEmissiveFactor = math::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    pPBRMaterialParam->mParams.mBaseColorFactor = math::Vector4(1.0f, 0.0f, 0.0f, 1.0f);
    pPBRMaterialParam->mParams.mMetalicRoughnessFactor = math::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    pPBRMaterialParam->mParams.mSpecularGlossinessFactor = math::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
}

void ProcessMaterials(const json::object_t& material, PBRMaterialParameters *gltfMat, std::map<std::string, int> &textureIdx)
{
    // Load material constants
    json::array_t ones = { 1.0, 1.0, 1.0, 1.0 };
    json::array_t zeroes = { 0.0, 0.0, 0.0, 0.0 };
    gltfMat->mDoubleSided = GetElementBoolean(material, "doubleSided", false);
    gltfMat->mBlending = GetElementString(material, "alphaMode", "OPAQUE") == "BLEND";
    gltfMat->mParams.mEmissiveFactor = GetVector(GetElementJsonArray(material, "emissiveFactor", zeroes));

    gltfMat->mDefines["DEF_doubleSided"] = std::to_string(gltfMat->mDoubleSided ? 1 : 0);
    gltfMat->mDefines["DEF_alphaCutoff"] = std::to_string(GetElementFloat(material, "alphaCutoff", 0.5));
    gltfMat->mDefines["DEF_alphaMode_" + GetElementString(material, "alphaMode", "OPAQUE")] = std::to_string(1);

    // look for textures and store their IDs in a map 
    //
    int index, texCoord;

    if (ProcessGetTextureIndexAndTextCoord(material, "normalTexture", &index, &texCoord))
    {
        textureIdx["normalTexture"] = index;
        gltfMat->mDefines["ID_normalTexCoord"] = std::to_string(texCoord);
    }

    if (ProcessGetTextureIndexAndTextCoord(material, "emissiveTexture", &index, &texCoord))
    {
        textureIdx["emissiveTexture"] = index;
        gltfMat->mDefines["ID_emissiveTexCoord"] = std::to_string(texCoord);
    }

    if (ProcessGetTextureIndexAndTextCoord(material, "occlusionTexture", &index, &texCoord))
    {
        textureIdx["occlusionTexture"] = index;
        gltfMat->mDefines["ID_occlusionTexCoord"] = std::to_string(texCoord);
    }

    // If using pbrMetallicRoughness
    //
    auto pbrMetallicRoughnessIt = material.find("pbrMetallicRoughness");
    if (pbrMetallicRoughnessIt != material.end())
    {
        const json &pbrMetallicRoughness = pbrMetallicRoughnessIt->second;

        gltfMat->mDefines["MATERIAL_METALLICROUGHNESS"] = "1";

        float metallicFactor = GetElementFloat(pbrMetallicRoughness, "metallicFactor", 1.0);
        float roughnessFactor = GetElementFloat(pbrMetallicRoughness, "roughnessFactor", 1.0);
        gltfMat->mParams.mMetalicRoughnessFactor = math::Vector4(metallicFactor, roughnessFactor, 0, 0);
        gltfMat->mParams.mBaseColorFactor = GetVector(GetElementJsonArray(pbrMetallicRoughness, "baseColorFactor", ones));

        if (ProcessGetTextureIndexAndTextCoord(pbrMetallicRoughness, "baseColorTexture", &index, &texCoord))
        {
            textureIdx["baseColorTexture"] = index;
            gltfMat->mDefines["ID_baseTexCoord"] = std::to_string(texCoord);
        }

        if (ProcessGetTextureIndexAndTextCoord(pbrMetallicRoughness, "metallicRoughnessTexture", &index, &texCoord))
        {
            textureIdx["metallicRoughnessTexture"] = index;
            gltfMat->mDefines["ID_metallicRoughnessTexCoord"] = std::to_string(texCoord);
        }
    }
    else
    {
        // If using KHR_materials_pbrSpecularGlossiness
        //
        auto extensionsIt = material.find("extensions");
        if (extensionsIt != material.end())
        {
            const json &extensions = extensionsIt->second;
            auto KHR_materials_pbrSpecularGlossinessIt = extensions.find("KHR_materials_pbrSpecularGlossiness");
            if (KHR_materials_pbrSpecularGlossinessIt != extensions.end())
            {
                const json &pbrSpecularGlossiness = KHR_materials_pbrSpecularGlossinessIt.value();

                gltfMat->mDefines["MATERIAL_SPECULARGLOSSINESS"] = "1";

                float glossiness = GetElementFloat(pbrSpecularGlossiness, "glossinessFactor", 1.0);
                gltfMat->mParams.mDiffuseFactor = GetVector(GetElementJsonArray(pbrSpecularGlossiness, "diffuseFactor", ones));
                gltfMat->mParams.mSpecularGlossinessFactor = math::Vector4(GetVector(GetElementJsonArray(pbrSpecularGlossiness, "specularFactor", ones)).getXYZ(), glossiness);

                if (ProcessGetTextureIndexAndTextCoord(pbrSpecularGlossiness, "diffuseTexture", &index, &texCoord))
                {
                    textureIdx["diffuseTexture"] = index;
                    gltfMat->mDefines["ID_diffuseTexCoord"] = std::to_string(texCoord);
                }

                if (ProcessGetTextureIndexAndTextCoord(pbrSpecularGlossiness, "specularGlossinessTexture", &index, &texCoord))
                {
                    textureIdx["specularGlossinessTexture"] = index;
                    gltfMat->mDefines["ID_specularGlossinessTexCoord"] = std::to_string(texCoord);
                }
            }
        }
    }
}

bool DoesMaterialUseSemantic(DefineList &defines, const std::string semanticName)
{
    // search if any *TexCoord mentions this channel
    //
    if (semanticName.substr(0, 9) == "TEXCOORD_")
    {
        char id = semanticName[9];

        for (const auto & def : defines)
        {
            auto size = static_cast<uint32_t>(def.first.size());
            if (size<= 8)
                continue;

            if (def.first.substr(size-8) == "TexCoord")
            {
                if (id == def.second.c_str()[0])
                {
                    return true;
                }
            }
        }
        return false;
    }

    return false;
}

bool ProcessGetTextureIndexAndTextCoord(
    const json::object_t& material,
    const std::string &textureName, int *pIndex, int *pTexCoord)
{
    if (pIndex)
    {
        *pIndex = -1;
        std::string strIndex = textureName + "/index";
        *pIndex = GetElementInt(material, strIndex.c_str(), -1);
        if (*pIndex == -1)
            return false;
    }

    if (pTexCoord)
    {
        *pTexCoord = -1;
        std::string strTexCoord = textureName + "/texCoord";
        *pTexCoord = GetElementInt(material, strTexCoord.c_str(), 0);
    }

    return true;
}

//
// Identify what material uses this texture, this helps:
// 1) determine the color space if the texture and also the cutout level. Authoring software saves albedo and emissive images in SRGB mode, the rest are linear mode
// 2) tell the cutOff value, to prevent thinning of alpha tested PNGs when lower mips are used.
//
void GetSrgbAndCutOffOfImageGivenItsUse(int imageIndex, const json &materials, bool *pSrgbOut, float *pCutoff)
{
    *pSrgbOut = false;
    *pCutoff = 1.0f; // no cutoff

    for (int m = 0; m < materials.size(); m++)
    {
        const json &material = materials[m];

        if (GetElementInt(material, "pbrMetallicRoughness/baseColorTexture/index", -1) == imageIndex)
        {
            *pSrgbOut = true;
            *pCutoff = GetElementFloat(material, "alphaCutoff", 0.5);
            return;
        }

        if (GetElementInt(material, "extensions/KHR_materials_pbrSpecularGlossiness/specularGlossinessTexture/index", -1) == imageIndex)
        {
            *pSrgbOut = true;
            return;
        }

        if (GetElementInt(material, "extensions/KHR_materials_pbrSpecularGlossiness/diffuseTexture/index", -1) == imageIndex)
        {
            *pSrgbOut = true;
            return;
        }

        if (GetElementInt(material, "emissiveTexture/index", -1) == imageIndex)
        {
            *pSrgbOut = true;
            return;
        }
    }
}
