#pragma once

#include "PCH.h"

class gltfAccessor
{
public:
    const void * mData{};
    int mCount{};
    int mStride{};
    int mDimension{};
    int mType{};

    math::Vector4 mMin{};
    math::Vector4 mMax{};

    const void *Get(int i) const
    {
        if (i >= mCount) i = mCount - 1;
        return (const char*)mData + mStride * i;
    }

    int FindClosetFloatIndex(float val) const
    {
        int ini = 0;
        int fin = mCount - 1;

        while (ini <= fin)
        {
            int mid = (ini + fin) / 2;
            float v = *(const float*)Get(mid);
            if (val < v) fin = mid - 1;
            else if (val > v) ini = mid + 1;
            else return mid;
        }
        return fin;
    }
};

struct gltfPrimitives
{
    math::Vector4 mCenter;
    math::Vector4 mRadius;
};

struct gltfMesh
{
    std::vector<gltfPrimitives> m_pPrimitives;
};

struct Transform
{
    math::Matrix4 mRotation = math::Matrix4::identity();
    math::Vector4 mTranslation = math::Vector4(0, 0, 0, 0);
    math::Vector4 mScale = math::Vector4(1, 1, 1, 0);

    void LookAt(math::Vector4 source, math::Vector4 target)
    {
        mRotation = math::inverse(math::Matrix4::lookAt(math::Point3(source.getXYZ()), math::Point3(target.getXYZ()), math::Vector3(0, 1, 0)));
        mTranslation = mRotation.getCol(3);  mRotation.setCol(3, math::Vector4(0, 0, 0, 1));
    }

    math::Matrix4 GetWorldMat() const
    {
        return math::Matrix4::translation(mTranslation.getXYZ()) * mRotation * math::Matrix4::scale(mScale.getXYZ());
    }
};

typedef int gltfNodeIdx;

struct gltfNode
{
    std::vector<gltfNodeIdx> mChildren;

    int skinIndex = -1;
    int meshIndex = -1;
    int channel = -1;
    bool bIsJoint = false;

    std::string mName;

    Transform mTransform;
};

struct NodeMatrixPostTransform
{
    gltfNode *pN; 
    math::Matrix4 m;
};

struct gltfScene
{
    std::vector<gltfNodeIdx> mNodes;
};

struct gltfSkins
{
    gltfAccessor mInverseBindMatrices;
    gltfNode *m_pSkeleton{};
    std::vector<int> mJointsNodeIdx;
};

class gltfSampler
{
public:
    void SampleLinear(float time, float* frac, float** pCurr, float** pNext) const
    {
        int currIdx = mTime.FindClosetFloatIndex(time);
        int nextIdx = std::min<int>(currIdx + 1, mTime.mCount - 1);

        if (currIdx < 0) currIdx++;

        if (currIdx == nextIdx)
        {
            *frac = 0;
            *pCurr = (float*)mValue.Get(currIdx);
            *pNext = (float*)mValue.Get(nextIdx);
            return;
        }

        float currTime = *(float*)mTime.Get(currIdx);
        float nextTime = *(float*)mTime.Get(nextIdx);

        *pCurr = (float*)mValue.Get(currIdx);
        *pNext = (float*)mValue.Get(nextIdx);
        *frac = (time - currTime) / (nextTime - currTime);
        assert(*frac >= 0 && *frac <= 1.0);
    }

public:
    gltfAccessor mTime;
    gltfAccessor mValue;
};

class gltfChannel
{
public:
    ~gltfChannel()
    {
        delete m_pTranslation;
        delete m_pRotation;
        delete m_pScale;
    }

public:
    gltfSampler * m_pTranslation;
    gltfSampler * m_pRotation;
    gltfSampler * m_pScale;
};

struct gltfAnimation
{
    float mDuration;
    std::map<int, gltfChannel> mChannels;
};

struct gltfLight
{
    enum LightType { LIGHT_DIRECTIONAL, LIGHT_POINTLIGHT, LIGHT_SPOTLIGHT };

    LightType mType = LIGHT_DIRECTIONAL;

    //tfNodeIdx m_nodeIndex = -1;

    math::Vector4 mColor;
    float       mRange;
    float       mIntensity = 0.0f;
    float       mInnerConeAngle = 0.0f;
    float       mOuterConeAngle = 0.0f;
    uint32_t    mShadowResolution = 1024;
    float       mBias = 70.0f / 100000.0f;
};

struct LightInstance
{
    int mLightId = -1;
    gltfNodeIdx mNodeIndex = -1;
};

struct gltfCamera
{
    enum LightType { CAMERA_PERSPECTIVE };
    float yFov{}, zFar{}, zNear{};

    gltfNodeIdx mNodeIndex = -1;
};