#include "utils.h"
#ifdef WIN32
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>
#endif

namespace gpu {

std::wstring getAssetsPathW() {
#ifdef WIN32
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlashIndex = path.find_last_of(L"\\/");
    std::wstring assetPath = path.substr(0, lastSlashIndex + 1);
    return assetPath;
#endif
    return L"";
}

std::string getAssetsPath() {
#ifdef WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string path(exePath);
    size_t lastSlashIndex = path.find_last_of("\\/");
    std::string assetPath = path.substr(0, lastSlashIndex + 1);
    return assetPath;
#endif
    return "";
}


uint32_t align256(uint32_t x) {
    return (x + 255) & ~255;
}
}