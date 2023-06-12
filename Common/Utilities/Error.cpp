#include "Error.h"

void ShowErrorMessageBox(LPCWSTR lpErrorString)
{
    int msgboxID = MessageBoxW(nullptr, lpErrorString, L"Error", MB_OK);
}

void ShowCustomErrorMessageBox(_In_opt_ LPCWSTR lpErrorString)
{
    int msgboxID = MessageBoxW(nullptr, lpErrorString, L"Error", MB_OK | MB_TOPMOST);
}