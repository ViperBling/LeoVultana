#include <D3DCompiler.h>
#include <dxcapi.h>

#include "DXCHelper.h"
#include "Misc.h"
#include "ShaderCompiler.h"
#include "ShaderCompilerCache.h"
#include "Error.h"

#define USE_DXC_SPIRV_FROM_DISK

void CompileMacros(const DefineList* pMacros, std::vector<D3D_SHADER_MACRO>* pOut)
{
    if (pMacros != nullptr)
    {
        for (auto it = pMacros->begin(); it != pMacros->end(); it++)
        {
            D3D_SHADER_MACRO macro;
            macro.Name = it->first.c_str();
            macro.Definition = it->second.c_str();
            pOut->push_back(macro);
        }
    }
}

DxcCreateInstanceProc sDXCCreateFunc;

bool InitDirectXCompiler()
{
    std::string fullshaderCompilerPath = "dxcompiler.dll";
    std::string fullshaderDXILPath = "dxil.dll";

    HMODULE dxil_module = ::LoadLibrary(fullshaderDXILPath.c_str());

    HMODULE dxc_module = ::LoadLibrary(fullshaderCompilerPath.c_str());
    sDXCCreateFunc = (DxcCreateInstanceProc)::GetProcAddress(dxc_module, "DxcCreateInstance");

    return sDXCCreateFunc != nullptr;
}

interface Includer : public ID3DInclude
{
public:
    virtual ~Includer() = default;

    HRESULT STDMETHODCALLTYPE Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) override
    {
        std::string fullpath = GetShaderCompilerLibDir() + pFileName;
        return ReadFile(fullpath.c_str(), (char**)ppData, (size_t*)pBytes, false) ? S_OK : E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE Close(LPCVOID pData) override
    {
        free((void*)pData);
        return S_OK;
    }
};

interface IncluderDxc : public IDxcIncludeHandler
{
    IDxcLibrary *m_pLibrary;

public:
    IncluderDxc(IDxcLibrary *pLibrary) : m_pLibrary(pLibrary) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(const IID &, void **) { return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() { return 0; }
    ULONG STDMETHODCALLTYPE Release() { return 0; }
    HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob **ppIncludeSource)
    {
        char fullpath[1024];
        sprintf_s<1024>(fullpath, ("%s\\%S"), GetShaderCompilerLibDir().c_str(), pFilename);

        LPCVOID pData;
        size_t bytes;
        HRESULT hr = ReadFile(fullpath, (char**)&pData, (size_t*)&bytes, false) ? S_OK : E_FAIL;

        if (hr == E_FAIL)
        {
            // return the failure here instead of crashing on CreateBlobWithEncodingFromPinned
            // to be able to report the error to the output console
            return hr;
        }

        IDxcBlobEncoding *pSource;
        ThrowIfFailed(m_pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)pData, (UINT32)bytes, CP_UTF8, &pSource));

        *ppIncludeSource = pSource;

        return S_OK;
    }
};

bool DXCCompileToDXO(
    size_t hash,
    const char* pSrcCode,
    const DefineList* pDefines,
    const char* pEntryPoint,
    const char* pParams,
    char** outSpvData,
    size_t* outSpvSize)
{
    //detect output bytecode type (DXBC/SPIR-V) and use proper extension
    std::string filenameOut;
    {
        auto found = std::string(pParams).find("-spirv ");
        if (found == std::string::npos)
            filenameOut = GetShaderCompilerCacheDir() + format("\\%p.dxo", hash);
        else
            filenameOut = GetShaderCompilerCacheDir() + format("\\%p.spv", hash);
    }

#ifdef USE_DXC_SPIRV_FROM_DISK
    if (ReadFile(filenameOut.c_str(), outSpvData, outSpvSize, true) && *outSpvSize > 0)
    {
        //Trace(format("thread 0x%04x compile: %p disk\n", GetCurrentThreadId(), hash));
        return true;
    }
#endif

    // create hlsl file for shader compiler to compile
    //
    std::string filenameHlsl = GetShaderCompilerCacheDir() + format("\\%p.hlsl", hash);
    std::ofstream ofs(filenameHlsl, std::ofstream::out);
    ofs << pSrcCode;
    ofs.close();

    std::string filenamePdb = GetShaderCompilerCacheDir() + format("\\%p.lld", hash);

    // get defines
    //
    wchar_t names[50][128];
    wchar_t values[50][128];
    DxcDefine defines[50];
    int defineCount = 0;
    if (pDefines != nullptr)
    {
        for (auto it = pDefines->begin(); it != pDefines->end(); it++)
        {
            swprintf_s<128>(names[defineCount], L"%S", it->first.c_str());
            swprintf_s<128>(values[defineCount], L"%S", it->second.c_str());
            defines[defineCount].Name = names[defineCount];
            defines[defineCount].Value = values[defineCount];
            defineCount++;
        }
    }

    // check whether DXCompiler is initialized
    if (sDXCCreateFunc == nullptr)
    {
        Trace("Error: s_dxc_create_func() is null, have you called InitDirectXCompiler() ?");
        return false;
    }

    IDxcLibrary *pLibrary;
    ThrowIfFailed(sDXCCreateFunc(CLSID_DxcLibrary, IID_PPV_ARGS(&pLibrary)));

    IDxcBlobEncoding *pSource;
    ThrowIfFailed(pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)pSrcCode, (UINT32)strlen(pSrcCode), CP_UTF8, &pSource));

    IDxcCompiler2* pCompiler;
    ThrowIfFailed(sDXCCreateFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));

    IncluderDxc Includer(pLibrary);

    std::vector<LPCWSTR> ppArgs;
    // splits params string into an array of strings
    {

        char params[1024];
        strcpy_s<1024>(params, pParams);

        char *next_token;
        char *token = strtok_s(params, " ", &next_token);
        while (token != nullptr) {
            wchar_t wide_str[1024];
            swprintf_s<1024>(wide_str, L"%S", token);

            const size_t wide_str2_len = wcslen(wide_str) + 1;
            wchar_t *wide_str2 = (wchar_t *)malloc(wide_str2_len * sizeof(wchar_t));
            wcscpy_s(wide_str2, wide_str2_len, wide_str);
            ppArgs.push_back(wide_str2);

            token = strtok_s(NULL, " ", &next_token);
        }
    }

    wchar_t  pEntryPointW[256];
    swprintf_s<256>(pEntryPointW, L"%S", pEntryPoint);

    IDxcOperationResult *pResultPre;
    HRESULT res1 = pCompiler->Preprocess(pSource, L"", NULL, 0, defines, defineCount, &Includer, &pResultPre);
    if (res1 == S_OK)
    {
        Microsoft::WRL::ComPtr<IDxcBlob> pCode1;
        pResultPre->GetResult(pCode1.GetAddressOf());
        std::string preprocessedCode;
        preprocessedCode = "// dxc -E" + std::string(pEntryPoint) + " " + std::string(pParams) + " " + filenameHlsl + "\n\n";
        if (pDefines)
        {
            for (auto it = pDefines->begin(); it != pDefines->end(); it++)
                preprocessedCode += "#define " + it->first + " " + it->second + "\n";
        }
        preprocessedCode += std::string((char *)pCode1->GetBufferPointer());
        preprocessedCode += "\n";
        SaveFile(filenameHlsl.c_str(), preprocessedCode.c_str(), preprocessedCode.size(), false);

        IDxcOperationResult* pOpRes;
        HRESULT res;

        if (false)
        {
            Microsoft::WRL::ComPtr<IDxcBlob> pPDB;
            LPWSTR pDebugBlobName[1024];
            res = pCompiler->CompileWithDebug(pSource, nullptr, pEntryPointW, L"", ppArgs.data(), (UINT32)ppArgs.size(), defines, defineCount, &Includer, &pOpRes, pDebugBlobName, pPDB.GetAddressOf());

            // Setup the correct name for the PDB
            if (pPDB)
            {
                char pPDBName[1024];
                sprintf_s(pPDBName, "%s\\%ls", GetShaderCompilerCacheDir().c_str(), *pDebugBlobName);
                SaveFile(pPDBName, pPDB->GetBufferPointer(), pPDB->GetBufferSize(), true);
            }
        }
        else
        {
            res = pCompiler->Compile(pSource, nullptr, pEntryPointW, L"", ppArgs.data(), (UINT32)ppArgs.size(), defines, defineCount, &Includer, &pOpRes);
        }

        // Clean up allocated memory
        while (!ppArgs.empty())
        {
            LPCWSTR pWString = ppArgs.back();
            ppArgs.pop_back();
            free((void*)pWString);
        }

        pSource->Release();
        pLibrary->Release();
        pCompiler->Release();

        IDxcBlob *pResult = nullptr;
        IDxcBlobEncoding *pError = nullptr;
        if (pOpRes != nullptr)
        {
            pOpRes->GetResult(&pResult);
            pOpRes->GetErrorBuffer(&pError);
            pOpRes->Release();
        }

        if (pResult != nullptr && pResult->GetBufferSize() > 0)
        {
            *outSpvSize = pResult->GetBufferSize();
            *outSpvData = (char *)malloc(*outSpvSize);

            memcpy(*outSpvData, pResult->GetBufferPointer(), *outSpvSize);

            pResult->Release();

            // Make sure pError doesn't leak if it was allocated
            if (pError)
                pError->Release();

#ifdef USE_DXC_SPIRV_FROM_DISK
            SaveFile(filenameOut.c_str(), *outSpvData, *outSpvSize, true);
#endif
            return true;
        }
        else
        {
            IDxcBlobEncoding *pErrorUtf8;
            pLibrary->GetBlobAsUtf8(pError, &pErrorUtf8);

            Trace("*** Error compiling %p.hlsl ***\n", hash);

            std::string filenameErr = GetShaderCompilerCacheDir() + format("\\%p.err", hash);
            SaveFile(filenameErr.c_str(), pErrorUtf8->GetBufferPointer(), pErrorUtf8->GetBufferSize(), false);

            std::string errMsg = std::string((char*)pErrorUtf8->GetBufferPointer(), pErrorUtf8->GetBufferSize());
            Trace(errMsg);

            // Make sure pResult doesn't leak if it was allocated
            if (pResult)
                pResult->Release();

            pErrorUtf8->Release();
        }
    }

    return false;
}

