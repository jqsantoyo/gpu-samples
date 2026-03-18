#include "utils.h"
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>

namespace gpu {

std::wstring getAssetsPathW() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlashIndex = path.find_last_of(L"\\/");
    std::wstring assetPath = path.substr(0, lastSlashIndex + 1);
    return assetPath;
}

std::string getAssetsPath() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string path(exePath);
    size_t lastSlashIndex = path.find_last_of("\\/");
    std::string assetPath = path.substr(0, lastSlashIndex + 1);
    return assetPath;
}


uint32_t align256(uint32_t x) {
    return (x + 255) & ~255;
}

bool readFile(const char* filename, std::vector<uint8_t>& data) {
    std::string path = getAssetsPath() + filename;
    FILE* file;
    errno_t err = fopen_s(&file, path.c_str(), "rb");
    if (err != 0 || file == nullptr) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    data.resize(size);
    fread(data.data(), size, 1, file);
    fclose(file);
    return true;
}

}