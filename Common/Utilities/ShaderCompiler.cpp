#include "ShaderCompiler.h"
#include "Misc.h"

//
// Hash a string of source code and recurse over its #include files
//
size_t HashShaderString(const char *pRootDir, const char *pShader, size_t hash)
{
    hash = Hash(pShader, strlen(pShader), hash);

    const char *pch = pShader;
    while (*pch != 0)
    {
        if (*pch == '/') // parse comments
        {
            pch++;
            if (*pch != 0 && *pch == '/')
            {
                pch++;
                while (*pch != 0 && *pch != '\n')
                    pch++;
            }
            else if (*pch != 0 && *pch == '*')
            {
                pch++;
                while ( (*pch != 0 && *pch != '*') && (*(pch+1) != 0 && *(pch+1) != '/'))
                    pch++;
            }
        }
        else if (*pch == '#') // parse #include
        {
            pch++;
            const char include[] = "include";
            int i = 0;
            while ((*pch!= 0) && *pch == include[i])
            {
                pch++;
                i++;
            }

            if (i == strlen(include))
            {
                while (*pch != 0 && *pch == ' ')
                    pch++;

                if (*pch != 0 && *pch == '\"')
                {
                    pch++;
                    const char *pName = pch;

                    while (*pch != 0 && *pch != '\"')
                        pch++;

                    char includeName[1024];
                    strcpy_s<1024>(includeName, pRootDir);
                    strncat_s<1024>(includeName, pName, pch - pName);

                    pch++;

                    char *pShaderCode = nullptr;
                    if (ReadFile(includeName, &pShaderCode, nullptr, false))
                    {
                        hash = HashShaderString(pRootDir, pShaderCode, hash);
                        free(pShaderCode);
                    }
                }
            }
        }
        else
        {
            pch++;
        }
    }

    return hash;
}