#pragma once

#include "PCH.h"
#include "ShaderCompiler.h"

bool InitDirectXCompiler();

bool DXCCompileToDXO(
    size_t hash,
    const char* pSrcCode,
    const DefineList* pDefines,
    const char* pEntryPoint,
    const char* pParams,
    char** outSpvData,
    size_t* outSpvSize);