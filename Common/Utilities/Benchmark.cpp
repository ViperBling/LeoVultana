//
// Created by qiuso on 2023/5/11.
//

#include "Benchmark.h"
#include "Sequence.h"
#include "Misc.h"

struct Benchmark
{
    int warmUpFrames;
    int runningTimeWhenNoAnimation;
    FILE *f = nullptr;
    float timeStep;
    float time;
    float timeStart, timeEnd;
    int frame;
    bool exitWhenTimeEnds;
    int cameraId;

    GLTFCommon *m_pGltfLoader;

    BenchmarkSequence mSequence;

    bool mAnimationFound = false;
    bool mSaveHeaders = true;
    float mNextTime = 0;
};

static Benchmark bm;

static void SaveTimestamps(float time, const std::vector<TimeStamp> &timeStamps)
{
    if (bm.mSaveHeaders)
    {
        //save headers
        fprintf(bm.f, "time");
        for (uint32_t i = 0; i < timeStamps.size(); i++)
            fprintf(bm.f, ", %s", timeStamps[i].mLabel.c_str());
        fprintf(bm.f, "\n");
        time = 0.0f;

        bm.mSaveHeaders = false;
    }

    //save timings
    fprintf(bm.f, "%f", time);
    for (uint32_t i = 0; i < timeStamps.size(); i++)
        fprintf(bm.f, ", %f", (float)timeStamps[i].mMicroseconds);
    fprintf(bm.f, "\n");
}

//
//
//
void BenchmarkConfig(const json& benchmark, int cameraId, GLTFCommon *pGltfLoader, const std::string& deviceName, const std::string& driverVersion)
{
    if (benchmark.is_null())
    {
        Trace("Benchmark section not found in json, the scene needs a benchmark section for this to work\n");
        exit(0);
    }

    bm.f = NULL;
    bm.frame = 0;
    // the number of frames to run before the benchmark starts
    bm.warmUpFrames = benchmark.value("warmUpFrames", 200);
    bm.exitWhenTimeEnds = benchmark.value("exitWhenTimeEnds", true);

    //get filename and open it
    std::string resultsFilename = benchmark.value("resultsFilename", "res.csv");
    bm.mSaveHeaders = true;
    if(fopen_s(&bm.f, resultsFilename.c_str(), "w") != 0)
    {
        Trace(format("The file %s cannot be opened\n", resultsFilename.c_str()));
        exit(0);
    }

    fprintf(bm.f, "#deviceName %s\n", deviceName.c_str());
    fprintf(bm.f, "#driverVersion %s\n", driverVersion.c_str());

    bm.timeStep = benchmark.value("timeStep", 1.0f);

    // Set default timeStart/timeEnd
    bm.timeStart = 0;
    bm.timeEnd = 0;
    if ((pGltfLoader!=NULL) && (pGltfLoader->mAnimations.size() > 0))
    {
        //if there is an animation take the endTime from the animation
        bm.timeEnd = pGltfLoader->mAnimations[0].mDuration;
    }

    //override those values if set
    bm.timeStart = benchmark.value("timeStart", bm.timeStart);
    bm.timeEnd = benchmark.value("timeEnd", bm.timeEnd);
    bm.time = bm.timeStart;

    // Sets the camera and its animation:
    //
    bm.mAnimationFound = false;
    bm.cameraId = cameraId;
    if ((pGltfLoader == NULL) || cameraId==-1)
    {
        if (benchmark.find("sequence") != benchmark.end())
        {
            //camera will use the sequence
            const json& sequence = benchmark["sequence"];
            bm.mSequence.ReadKeyframes(sequence, bm.timeStart, bm.timeEnd);
            bm.timeStart = bm.mSequence.GetTimeStart();
            bm.timeEnd = bm.mSequence.GetTimeEnd();
            bm.mAnimationFound = true;
            bm.cameraId = -1;
        }
        else
        {
            // will use no sequence, will use the default static camera
        }
    }
    else
    {
        // a camera from the GLTF will be used
        // check such a camera exist, otherwise show an error and quit
        Camera Cam;
        if (pGltfLoader->GetCamera(cameraId, &Cam) == false)
        {
            Trace(format("The cameraId %i doesn't exist in the GLTF\n", cameraId));
            exit(0);
        }
        bm.mAnimationFound = true;
    }

    bm.mNextTime = 0;
    bm.m_pGltfLoader = pGltfLoader;
}

float BenchmarkLoop(const std::vector<TimeStamp> &timeStamps, Camera *pCam, std::string& outScreenShotName)
{
    if (bm.frame < bm.warmUpFrames) // warmup
    {
        bm.frame++;
        return bm.time;
    }

    if (bm.time > bm.timeEnd) // are we done yet?
    {
        fclose(bm.f);

        if (bm.exitWhenTimeEnds)
        {
            PostQuitMessage(0);
            return bm.time;
        }
    }

    SaveTimestamps(bm.time, timeStamps);

    // animate camera
    if (bm.mAnimationFound && (pCam != NULL))
    {
        // if GLTF has camera with cameraID then use that camera and its animation
        if (bm.cameraId >= 0)
        {
            bm.m_pGltfLoader->GetCamera(bm.cameraId, pCam);
        }
        else
        {
            // cameraID is -1, then use our sequence
            if (bm.time >= bm.mNextTime)
            {
                bm.mNextTime = bm.mSequence.GetNextKeyTime(bm.time);

                const BenchmarkSequence::KeyFrame keyFrame = bm.mSequence.GetNextKeyFrame(bm.time);

                const bool bValidKeyframe = keyFrame.mTime >= 0;
                if (bValidKeyframe)
                {
                    if (keyFrame.mCamera == -1)
                    {
                        pCam->LookAt(keyFrame.mFrom, keyFrame.mTo);
                    }
                    else
                    {
                        bm.m_pGltfLoader->GetCamera(keyFrame.mCamera, pCam);
                    }

                    const bool bShouldTakeScreenshot = !keyFrame.mScreenShotName.empty();
                    if (bShouldTakeScreenshot)
                    {
                        outScreenShotName = keyFrame.mScreenShotName;
                    }
                }
            }
        }
    }

    float time = bm.time;
    bm.time += bm.timeStep;
    return time;
}


