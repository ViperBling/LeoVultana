//
// Created by Administrator on 2023/5/12.
//

#include "Sequence.h"

void BenchmarkSequence::ReadKeyframes(const json& jSequence, float timeStart, float timeEnd)
{
    mTimeStart = jSequence.value("timeStart", timeStart);
    mTimeEnd = jSequence.value("timeEnd", timeEnd);

    mKeyFrames.clear();

    const json& keyFrames = jSequence["keyFrames"];
    for (const json& jkf : keyFrames)
    {
        KeyFrame key = {};
        key.mCamera = -1;

        key.mTime = jkf["time"];

        auto find = jkf.find("camera");
        if(find != jkf.cend())
        {
            key.mCamera = find.value();
        }

        if (key.mCamera == -1)
        {
            const json& jfrom = jkf["from"];
            key.mFrom = math::Vector4(jfrom[0], jfrom[1], jfrom[2], 0);

            const json& jto = jkf["to"];
            key.mTo = math::Vector4(jto[0], jto[1], jto[2], 0);
        }

        key.mScreenShotName = jkf.value("screenShotName", "");

        mKeyFrames.push_back(key);
    }
}

float BenchmarkSequence::GetNextKeyTime(float time)
{
    for (int i = 0; i < mKeyFrames.size(); i++)
    {
        KeyFrame key = mKeyFrames[i];
        if (key.mTime > time)
        {
            return key.mTime;
        }
    }

    return -1;
}

BenchmarkSequence::KeyFrame BenchmarkSequence::GetNextKeyFrame(float time) const
{
    for (int i = 0; i < mKeyFrames.size(); i++)
    {
        const KeyFrame& key = mKeyFrames[i];
        if (key.mTime >= time)
        {
            return key;
        }
    }

    // key frame not found, return empty
    KeyFrame invalidKey = KeyFrame{ 0 };
    invalidKey.mTime = -1.0f;
    return invalidKey;
}
