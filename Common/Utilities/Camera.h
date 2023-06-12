#pragma once

#include "PCH.h"
#include "vectormath/vectormath.hpp"

class Camera
{
public:
    Camera();
    void SetMatrix(const math::Matrix4& cameraMatrix);
    void LookAt(const math::Vector4& eyePos, const math::Vector4& lookAt);
    void LookAt(float yaw, float pitch, float distance, const math::Vector4& at);
    void SetFov(float fov, uint32_t width, uint32_t height, float nearPlane, float farPlane);
    void SetFov(float fov, float aspectRatio, float nearPlane, float farPlane);
    void UpdateCameraPolar(float yaw, float pitch, float x, float y, float distance);
    void UpdateCameraWASD(float yaw, float pitch, const bool keyDown[256], double deltaTime);

    math::Matrix4 GetView() const { return mView; }
    math::Matrix4 GetPrevView() const { return mPrevView; }
    math::Vector4 GetPosition() const { return mEyePos;   }


    math::Vector4 GetDirection()    const { return math::Vector4((math::transpose(mView) * math::Vector4(0.0f, 0.0f, 1.0f, 0.0f)).getXYZ(), 0); }
    math::Vector4 GetUp()           const { return math::Vector4((math::transpose(mView) * math::Vector4(0.0f, 1.0f, 0.0f, 0.0f)).getXYZ(), 0); }
    math::Vector4 GetSide()         const { return math::Vector4((math::transpose(mView) * math::Vector4(1.0f, 1.0f, 0.0f, 0.0f)).getXYZ(), 0); }
    math::Matrix4 GetProjection()   const { return mProj; }

    float GetFovH() const { return mFovH; }
    float GetFovV() const { return mFovV; }

    float GetAspectRatio() const { return mAspectRatio; }

    float GetNearPlane() const { return mNear; }
    float GetFarPlane() const { return mFar; }

    float GetYaw() const { return mYaw; }
    float GetPitch() const { return mPitch; }
    float GetDistance() const { return mDistance; }

    void SetSpeed( float speed ) { mSpeed = speed; }
    void SetProjectionJitter(float jitterX, float jitterY);
    void SetProjectionJitter(uint32_t width, uint32_t height, uint32_t &seed);
    void UpdatePreviousMatrices() { mPrevView = mView; }

private:
    math::Matrix4       mView{};
    math::Matrix4       mProj{};
    math::Matrix4       mPrevView{};
    math::Vector4       mEyePos{};
    float               mDistance{};
    float               mFovV{}, mFovH{};
    float               mNear{}, mFar{};
    float               mAspectRatio{};

    float               mSpeed = 1.0f;
    float               mYaw = 0.0f;
    float               mPitch = 0.0f;
    float               mRoll = 0.0f;
};

math::Vector4 PolarToVector(float yaw, float pitch);
math::Matrix4 LookAtRH(const math::Vector4& eyePos, const math::Vector4& lookAt);
math::Vector4 MoveWASD(const bool keyDown[256]);
