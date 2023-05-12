#pragma once

#include "PCH.h"
#include "json.h"
#include "Camera.h"
#include "GLTFStructures.h"

using json = nlohmann::json;

// Define a maximum number of shadows supported in a scene (note, these are only for spots and directional)
static const uint32_t MaxLightInstances = 80;
static const uint32_t MaxShadowInstances = 32;

class Matrix2
{
    math::Matrix4 mCurrent;
    math::Matrix4 mPrevious;
public:
    void Set(const math::Matrix4& m) { mPrevious = mCurrent; mCurrent = m; }
    math::Matrix4 GetCurrent() const { return mCurrent; }
    math::Matrix4 GetPrevious() const { return mPrevious; }
};

//
// Structures holding the per frame constant buffer data.
//
struct Light
{
    math::Matrix4 mLightViewProj;
    math::Matrix4 mLightView;

    float         direction[3];
    float         range;

    float         color[3];
    float         intensity;

    float         position[3];
    float         innerConeCos;

    float         outerConeCos;
    uint32_t      type;
    float         depthBias;
    int32_t       shadowMapIndex = -1;
};

const uint32_t LightType_Directional = 0;
const uint32_t LightType_Point = 1;
const uint32_t LightType_Spot = 2;

struct PerFrame
{
    math::Matrix4 mCameraCurrViewProj;
    math::Matrix4 mCameraPrevViewProj;
    math::Matrix4 mInverseCameraCurrViewProj;
    math::Vector4 cameraPos;
    float     iblFactor;
    float     emissiveFactor;
    float     invScreenResolution[2];

    math::Vector4 wireframeOptions;
    float     lodBias = 0.0f;
    uint32_t  padding[2];
    uint32_t  lightCount;
    Light     lights[MaxLightInstances];
};

//
// GLTFCommon, common stuff that is API agnostic
//
class GLTFCommon
{
public:
    bool Load(const std::string &path, const std::string &filename);
    void Unload();

    // misc functions
    int FindMeshSkinId(int meshId) const;
    int GetInverseBindMatricesBufferSizeByID(int id) const;
    void GetBufferDetails(int accessor, gltfAccessor *pAccessor) const;
    void GetAttributesAccessors(const json &gltfAttributes, std::vector<char*> *pStreamNames, std::vector<gltfAccessor> *pAccessors) const;

    // transformation and animation functions
    void SetAnimationTime(uint32_t animationIndex, float time);
    void TransformScene(int sceneIndex, const math::Matrix4& world);
    PerFrame *SetPerFrameData(const Camera& cam);
    bool GetCamera(uint32_t cameraIdx, Camera *pCam) const;
    gltfNodeIdx AddNode(const gltfNode& node);
    int AddLight(const gltfNode& node, const gltfLight& light);

private:
    void InitTransformedData(); //this is called after loading the data from the GLTF
    void TransformNodes(const math::Matrix4& world, const std::vector<gltfNodeIdx> *pNodes);
    math::Matrix4 ComputeDirectionalLightOrthographicMatrix(const math::Matrix4& mLightView);

public:
public:
    json j3;

    std::string mPath;
    std::vector<gltfScene> mScenes;
    std::vector<gltfMesh> mMeshes;
    std::vector<gltfSkins> mSkins;
    std::vector<gltfLight> mLights;
    std::vector<LightInstance> mLightInstances;
    std::vector<gltfCamera> mCameras;

    std::vector<gltfNode> mNodes;

    std::vector<gltfAnimation> mAnimations;
    std::vector<char *> mBuffersData;

    const json *m_pAccessors;
    const json *m_pBufferViews;

    std::vector<math::Matrix4> mAnimatedMats;       // object space matrices of each node after being animated

    std::vector<Matrix2> mWorldSpaceMats;     // world space matrices of each node after processing the hierarchy
    std::map<int, std::vector<Matrix2>> mWorldSpaceSkeletonMats; // skinning matrices, following the m_jointsNodeIdx order

    PerFrame mPerFrameData;

};