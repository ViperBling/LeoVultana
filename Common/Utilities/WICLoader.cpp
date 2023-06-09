#include "PCH.h"
#include "WICLoader.h"
#include "Misc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//#define USE_WIC

#ifdef USE_WIC
#include "wincodec.h"
static IWICImagingFactory *m_pWICFactory = nullptr;
#endif

WICLoader::~WICLoader()
{
    free(m_pData);
}

bool WICLoader::Load(const char *pFilename, float cutOff, IMG_INFO *pInfo)
{
#ifdef USE_WIC
    HRESULT hr = S_OK;

    if (m_pWICFactory == NULL)
    {
        hr = CoInitialize(NULL);
        assert(SUCCEEDED(hr));
        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pWICFactory));
        assert(SUCCEEDED(hr));
    }

    IWICStream* pWicStream;
    hr = m_pWICFactory->CreateStream(&pWicStream);
    assert(SUCCEEDED(hr));

    wchar_t  uniName[1024];
    swprintf(uniName, 1024, L"%S", pFilename);
    hr = pWicStream->InitializeFromFilename(uniName, GENERIC_READ);
    //assert(hr == S_OK);
    if (FAILED(hr))
        return false;

    IWICBitmapDecoder *pBitmapDecoder;
    hr = m_pWICFactory->CreateDecoderFromStream(pWicStream, NULL, WICDecodeMetadataCacheOnDemand, &pBitmapDecoder);
    assert(SUCCEEDED(hr));

    IWICBitmapFrameDecode *pFrameDecode;
    hr = pBitmapDecoder->GetFrame(0, &pFrameDecode);
    assert(SUCCEEDED(hr));

    IWICFormatConverter *pIFormatConverter;
    hr = m_pWICFactory->CreateFormatConverter(&pIFormatConverter);
    assert(SUCCEEDED(hr));

    hr = pIFormatConverter->Initialize( pFrameDecode, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 100.0, WICBitmapPaletteTypeCustom);
    assert(SUCCEEDED(hr));

    uint32_t width, height;
    pFrameDecode->GetSize(&width, &height);
    pFrameDecode->Release();

    int bufferSize = width * height * 4;
    m_pData = (char *)malloc(bufferSize);
    hr = pIFormatConverter->CopyPixels( NULL, width * 4, bufferSize, (BYTE*)m_pData);
    assert(SUCCEEDED(hr));
#else
    int32_t width, height, channels;
    m_pData = (char*)stbi_load(pFilename, &width, &height, &channels, STBI_rgb_alpha);
#endif

    // compute number of mips
    uint32_t mipWidth  = width;
    uint32_t mipHeight = height;
    uint32_t mipCount = 0;
    for(;;)
    {
        mipCount++;
        if (mipWidth > 1) mipWidth >>= 1;
        if (mipHeight > 1) mipHeight >>= 1;
        if (mipWidth == 1 && mipHeight == 1)
            break;
    }

    // fill img struct
    pInfo->arraySize = 1;
    pInfo->width = width;
    pInfo->height = height;
    pInfo->depth = 1;
    pInfo->mipMapCount = mipCount;
    pInfo->bitCount = 32;
    pInfo->format = DXGI_FORMAT_R8G8B8A8_UNORM;

    // if there is a cut off, compute the alpha test coverage of the top mip
    // mip generation will try to match this value so objects dont get thinner
    // as they use lower mips
    mCutOff = cutOff;
    if (mCutOff < 1.0f)
        mAlphaTestCoverage = GetAlphaCoverage(width, height, 1.0f, (int)(255 * mCutOff));
    else
        mAlphaTestCoverage = 1.0f;

#ifdef USE_WIC
    pIFormatConverter->Release();
    pBitmapDecoder->Release();
    pWicStream->Release();
#endif

    return true;
}

void WICLoader::CopyPixels(void *pDest, uint32_t stride, uint32_t bytesWidth, uint32_t height)
{
    for (uint32_t y = 0; y < height; y++)
    {
        memcpy((char*)pDest + y*stride, m_pData + y*bytesWidth, bytesWidth);
    }

    MipImage(bytesWidth / 4, height);
}

float WICLoader::GetAlphaCoverage(uint32_t width, uint32_t height, float scale, int cutoff) const
{
    double val = 0;

    auto *pImg = (uint32_t *)m_pData;

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            auto *pPixel = (uint8_t *)pImg++;

            int alpha = (int)(scale * (float)pPixel[3]);
            if (alpha > 255) alpha = 255;
            if (alpha <= cutoff)
                continue;

            val += alpha;
        }
    }

    return (float)(val / (height*width *255));
}

void WICLoader::ScaleAlpha(uint32_t width, uint32_t height, float scale)
{
    auto *pImg = (uint32_t *)m_pData;

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            auto *pPixel = (uint8_t *)pImg++;

            int alpha = (int)(scale * (float)pPixel[3]);
            if (alpha > 255) alpha = 255;

            pPixel[3] = alpha;
        }
    }
}

void WICLoader::MipImage(uint32_t width, uint32_t height)
{
    //compute mip so next call gets the lower mip
    int offsetsX[] = { 0,1,0,1 };
    int offsetsY[] = { 0,0,1,1 };

    auto *pImg = (uint32_t *)m_pData;

#define GetByte(color, component) (((color) >> (8 * (component))) & 0xff)
#define GetColor(ptr, x,y) (ptr[(x)+(y)*width])
#define SetColor(ptr, x,y, col) ptr[(x)+(y)*width/2]=col;

    for (uint32_t y = 0; y < height; y+=2)
    {
        for (uint32_t x = 0; x < width; x+=2)
        {
            uint32_t ccc = 0;
            for (uint32_t c = 0; c < 4; c++)
            {
                uint32_t cc = 0;
                for (uint32_t i = 0; i < 4; i++)
                    cc += GetByte(GetColor(pImg, x + offsetsX[i], y + offsetsY[i]), 3-c);

                ccc = (ccc << 8) | (cc/4);
            }
            SetColor(pImg, x / 2, y / 2, ccc);
        }
    }

    // For cutouts we need to scale the alpha channel to match the coverage of the top MIP map
    // otherwise cutouts seem to get thinner when smaller mips are used
    // Credits: http://www.ludicon.com/castano/blog/articles/computing-alpha-mipmaps/
    if (mAlphaTestCoverage < 1.0)
    {
        float ini = 0;
        float fin = 10;
        float mid;
        float alphaPercentage;
        int iter = 0;
        for(;iter<50;iter++)
        {
            mid = (ini + fin) / 2;
            alphaPercentage = GetAlphaCoverage(width / 2, height / 2, mid, (int)(mCutOff * 255));

            if (fabs(alphaPercentage - mAlphaTestCoverage) < .001)
                break;

            if (alphaPercentage > mAlphaTestCoverage)
                fin = mid;
            if (alphaPercentage < mAlphaTestCoverage)
                ini = mid;
        }
        ScaleAlpha(width / 2, height / 2, mid);
        //Trace(format("(%4i x %4i), %f, %f, %i\n", width, height, alphaPercentage, 1.0f, 0));
    }
}
