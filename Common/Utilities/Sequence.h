#pragma once

#include "PCH.h"
#include "json.h"
#include "Camera.h"
#include "GLTFCommon.h"

using json = nlohmann::json;

class BenchmarkSequence
{
public:
    struct KeyFrame
    {
        float mTime;
        int mCamera;
        math::Vector4 mFrom, mTo;
        std::string mScreenShotName;
    };

    void ReadKeyframes(const json& jSequence, float timeStart, float timeEnd);
    float GetTimeStart() const { return mTimeStart; }
    float GetTimeEnd() const { return mTimeEnd; }

    //
    // Given a time return the time of the next keyframe, that way we know how long we have to wait until we apply the next keyframe
    //
    float GetNextKeyTime(float time);
    KeyFrame GetNextKeyFrame(float time) const;

private:
    float mTimeStart;
    float mTimeEnd;

    std::vector<KeyFrame> mKeyFrames;
};
