#pragma once

#include "Utilities/PCH.h"
#include <DXGIFormat.h>
#include "ImgLoader.h"

class DDSLoader : public ImgLoader
{
public:
    ~DDSLoader();
    bool Load(const char *pFilename, float cutOff, IMG_INFO *pInfo) override;
    // after calling Load, calls to CopyPixels return each time a lower mip level
    void CopyPixels(void *pDest, uint32_t stride, uint32_t width, uint32_t height) override;
private:
    HANDLE mHandle = INVALID_HANDLE_VALUE;
};