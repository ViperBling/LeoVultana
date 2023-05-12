#pragma once

#include "PCH.h"
#include "Misc.h"

void ShowErrorMessageBox(LPCWSTR lpErrorString);
void ShowCustomErrorMessageBox(_In_opt_ LPCWSTR lpErrorString);

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        wchar_t err[256];
        memset(err, 0, 256);
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
        char errA[256];
        size_t returnSize;
        wcstombs_s(&returnSize, errA, 255, err, 255);
        Trace(errA);
#ifdef _DEBUG
        ShowErrorMessageBox(err);
#endif
        throw 1;
    }
}
