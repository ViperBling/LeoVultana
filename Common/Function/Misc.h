#pragma once

#include "PCH.h"
#include "vectormath/vectormath.hpp"

static constexpr float AMD_PI = 3.1415926535897932384626433832795f;
static constexpr float AMD_PI_OVER_2 = 1.5707963267948966192313216916398f;
static constexpr float AMD_PI_OVER_4 = 0.78539816339744830961566084581988f;

double MillisecondsNow();
std::string format(const char* format, ...);
bool ReadFile(const char *name, char **data, size_t *size, bool isbinary);
bool SaveFile(const char *name, void const*data, size_t size, bool isbinary);
void Trace(const std::string &str);
void Trace(const char* pFormat, ...);
bool LaunchProcess(const char* commandLine, const char* filenameErr);
inline void GetXYZ(float *f, math::Vector4 v)
{
    f[0] = v.getX();
    f[1] = v.getY();
    f[2] = v.getZ();
}

bool CameraFrustumToBoxCollision(const math::Matrix4& mCameraViewProj, const math::Vector4& boxCenter, const math::Vector4& boxExtent);

struct AxisAlignedBoundingBox
{
    math::Vector4 m_min;
    math::Vector4 m_max;
    bool m_isEmpty;

    AxisAlignedBoundingBox();

    void Merge(const AxisAlignedBoundingBox& bb);
    void Grow(const math::Vector4 v);

    bool HasNoVolume() const;
};

AxisAlignedBoundingBox GetAABBInGivenSpace(const math::Matrix4& mTransform, const math::Vector4& boxCenter, const math::Vector4& boxExtent);

// align val to the next multiple of alignment
template<typename T> inline T AlignUp(T val, T alignment)
{
    return (val + alignment - (T)1) & ~(alignment - (T)1);
}
// align val to the previous multiple of alignment
template<typename T> inline T AlignDown(T val, T alignment)
{
    return val & ~(alignment - (T)1);
}
template<typename T> inline T DivideRoundingUp(T a, T b)
{
    return (a + b - (T)1) / b;
}

class Profile
{
    double m_startTime;
    const char *m_label;
public:
    explicit Profile(const char *label) {
        m_startTime = MillisecondsNow(); m_label = label;
    }
    ~Profile() {
        Trace(format("*** %s  %f ms\n", m_label, (MillisecondsNow() - m_startTime)));
    }
};

class Log
{
public:
    static int InitLogSystem();
    static int TerminateLogSystem();
    static void Trace(const char* LogString);

private:
    Log();
    virtual ~Log();
    void Write(const char* LogString);

private:
#define MAX_INFLIGHT_WRITES 32
    static Log* m_pLogInstance;

    HANDLE mFileHandle = INVALID_HANDLE_VALUE;
    OVERLAPPED mOverlappedData[MAX_INFLIGHT_WRITES];
    uint32_t mCurrentIOBufferIndex = 0;
    uint32_t mWriteOffset = 0;
};

int countBits(uint32_t v);