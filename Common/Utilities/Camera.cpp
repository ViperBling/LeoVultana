#include "PCH.h"
#include "Camera.h"

Camera::Camera()
{
    mView = math::Matrix4::identity();
    mEyePos = math::Vector4(0, 0, 0, 0);
    mDistance = -1;
}

void Camera::SetMatrix(const math::Matrix4 &cameraMatrix)
{
    mEyePos = cameraMatrix.getCol3();
    LookAt(mEyePos, mEyePos + cameraMatrix * math::Vector4(0, 0, 1, 0));
}

void Camera::LookAt(const math::Vector4 &eyePos, const math::Vector4 &lookAt)
{
    mEyePos = eyePos;
    mView = LookAtRH(eyePos, lookAt);
    mDistance = math::SSE::length(lookAt - eyePos);

    math::Vector4 zBasis = mView.getRow(2);
    mYaw = atan2f(zBasis.getX(), zBasis.getZ());
    float fLen = sqrtf(zBasis.getZ() * zBasis.getZ() + zBasis.getX() * zBasis.getX());
    mPitch = atan2f(zBasis.getY(), fLen);
}

void Camera::LookAt(float yaw, float pitch, float distance, const math::Vector4 &at)
{
    LookAt(at + PolarToVector(yaw, pitch) * distance, at);
}

void Camera::SetFov(float fov, uint32_t width, uint32_t height, float nearPlane, float farPlane)
{
    SetFov(fov, (float)width * 1.f / (float)height, nearPlane, farPlane);
}

void Camera::SetFov(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    mAspectRatio = aspectRatio;
    mNear = nearPlane;
    mFar = farPlane;

    mFovV = fov;
    mFovH = std::min<float>(mFovV * aspectRatio, XM_PI / 2.0f);

    mProj = math::Matrix4::perspective(fov, aspectRatio, nearPlane, farPlane);
}

//--------------------------------------------------------------------------------------
//
// UpdateCamera
//
//--------------------------------------------------------------------------------------
void Camera::UpdateCameraPolar(float yaw, float pitch, float x, float y, float distance)
{
    pitch = std::max(-XM_PIDIV2 + 1e-3f, std::min(pitch, XM_PIDIV2 - 1e-3f));

    // Trucks camera, moves the camera parallel to the view plane.
    mEyePos += GetSide() * x * distance / 10.0f;
    mEyePos += GetUp() * y * distance / 10.0f;

    // Orbits camera, rotates a camera about the target
    math::Vector4 dir = GetDirection();
    math::Vector4 pol = PolarToVector(yaw, pitch);

    math::Vector4 at = mEyePos - (dir * mDistance);

    LookAt(at + (pol * distance), at);
}

void Camera::UpdateCameraWASD(float yaw, float pitch, const bool *keyDown, double deltaTime)
{
    mEyePos += math::transpose(mView) * (MoveWASD(keyDown) * mSpeed * (float)deltaTime);
    math::Vector4 dir = PolarToVector(yaw, pitch) * mDistance;
    LookAt(GetPosition(), GetPosition() - dir);
}

//--------------------------------------------------------------------------------------
//
// SetProjectionJitter
//
//--------------------------------------------------------------------------------------
void Camera::SetProjectionJitter(float jitterX, float jitterY)
{
    math::Vector4 proj = mProj.getCol2();
    proj.setX(jitterX);
    proj.setY(jitterY);
    mProj.setCol2(proj);
}

void Camera::SetProjectionJitter(uint32_t width, uint32_t height, uint32_t &seed)
{
    static const auto CalculateHaltonNumber = [](uint32_t index, uint32_t base)
    {
        float f = 1.0f, result = 0.0f;

        for (uint32_t i = index; i > 0;)
        {
            f /= static_cast<float>(base);
            result = result + f * static_cast<float>(i % base);
            i = static_cast<uint32_t>(floorf(static_cast<float>(i) / static_cast<float>(base)));
        }
        return result;
    };

    seed = (seed + 1) % 16;   // 16x TAA

    float jitterX = 2.0f * CalculateHaltonNumber(seed + 1, 2) - 1.0f;
    float jitterY = 2.0f * CalculateHaltonNumber(seed + 1, 3) - 1.0f;

    jitterX /= static_cast<float>(width);
    jitterY /= static_cast<float>(height);

    SetProjectionJitter(jitterX, jitterY);
}

//--------------------------------------------------------------------------------------
//
// Get a vector pointing in the direction of yaw and pitch
//
//--------------------------------------------------------------------------------------
math::Vector4 PolarToVector(float yaw, float pitch)
{
    return {sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0};
}

math::Matrix4 LookAtRH(const math::Vector4& eyePos, const math::Vector4& lookAt)
{
    return math::Matrix4::lookAt(math::toPoint3(eyePos), math::toPoint3(lookAt), math::Vector3(0, 1, 0));
}

math::Vector4 MoveWASD(const bool keyDown[256])
{
    float scale = keyDown[VK_SHIFT] ? 5.0f : 1.0f;
    float x = 0, y = 0, z = 0;

    if (keyDown['W'])
    {
        z = -scale;
    }
    if (keyDown['S'])
    {
        z = scale;
    }
    if (keyDown['A'])
    {
        x = -scale;
    }
    if (keyDown['D'])
    {
        x = scale;
    }
    if (keyDown['E'])
    {
        y = scale;
    }
    if (keyDown['Q'])
    {
        y = -scale;
    }

    return {x, y, z, 0.0f};
}