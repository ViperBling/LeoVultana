#include "PCH.h"
#include "DDSLoader.h"
#include "WICLoader.h"
#include "ImgLoader.h"

ImgLoader *CreateImageLoader(const char *pFilename)
{
    // get the last 4 char (assuming this is the file extension)
    size_t len = strlen(pFilename);
    const char* ext = pFilename + (len - 4);
    if(_stricmp(ext, ".dds") == 0)
    {
        return new DDSLoader();
    }
    else
    {
        return new WICLoader();
    }
}
