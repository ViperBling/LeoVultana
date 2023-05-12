#pragma once

#include <cstdint>
#include <DXGIFormat.h>

struct IMG_INFO
{
    UINT32           width{};
    UINT32           height{};
    UINT32           depth{};
    UINT32           arraySize{};
    UINT32           mipMapCount{};
    DXGI_FORMAT      format{};
    UINT32           bitCount{};
};

class ImgLoader
{
public:
    virtual ~ImgLoader() = default;
    virtual bool Load(const char *pFilename, float cutOff, IMG_INFO *pInfo) = 0;
    // after calling Load, calls to CopyPixels return each time a lower mip level
    virtual void CopyPixels(void *pDest, uint32_t stride, uint32_t width, uint32_t height) = 0;
};

ImgLoader *CreateImageLoader(const char *pFilename);