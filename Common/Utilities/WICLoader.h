#pragma once

#include "ImgLoader.h"

// Loads a JPEGs, PNGs, BMPs and any image the Windows Imaging Component can load.
// It even applies some alpha scaling to prevent cutouts to fade away when lower mips are used.

class WICLoader : public ImgLoader
{
public:
    ~WICLoader();
    bool Load(const char *pFilename, float cutOff, IMG_INFO *pInfo);
    // after calling Load, calls to CopyPixels return each time a lower mip level
    void CopyPixels(void *pDest, uint32_t stride, uint32_t width, uint32_t height);

private:
    void MipImage(uint32_t width, uint32_t height);
    // scale alpha to prevent thinning when lower mips are used
    float GetAlphaCoverage(uint32_t width, uint32_t height, float scale, int cutoff) const;
    void ScaleAlpha(uint32_t width, uint32_t height, float scale);

    char *m_pData;

    float mAlphaTestCoverage;
    float mCutOff;
};


