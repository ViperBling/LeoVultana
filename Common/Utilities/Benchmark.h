#pragma once

#include "PCH.h"
#include "json.h"
#include "Camera.h"
#include "GLTF/GLTFCommon.h"

using json = nlohmann::json;

struct TimeStamp
{
    std::string mLabel;
    float mMicroseconds;
};

void BenchmarkConfig(const json& benchmark, int cameraId, GLTFCommon *pGltfLoader, const std::string& deviceName = "not set", const std::string& driverVersion = "not set");
float BenchmarkLoop(const std::vector<TimeStamp> &timeStamps, Camera *pCam, std::string& outScreenShotName);