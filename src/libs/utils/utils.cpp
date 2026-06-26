#include "utils.h"
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>
#include <directxmath.h>

using namespace DirectX;
namespace gpu {

uint32_t toUint(vec4 v) {
    return (static_cast<uint32_t>(255.0f * v.x) << 24)
         + (static_cast<uint32_t>(255.0f * v.y) << 16)
         + (static_cast<uint32_t>(255.0f * v.z) <<  8)
         +  static_cast<uint32_t>(255.0f * v.w);
}


void FreeList::init(int size) {
    allocated.resize(size);
    freeList.reserve(size);
}

void FreeList::reset() {
    nextFree = 0;
    count = 0;
    freeList.clear();
    std::fill(allocated.begin(), allocated.end(), false);
}

int FreeList::alloc() {
    if (freeList.size() > 0) {
        int idx = freeList.back();
        freeList.pop_back();
        allocated[idx] = true;
        count++;
        return idx;
    } else if (nextFree < freeList.capacity()) {
        int idx = nextFree++;
        allocated[idx] = true;
        count++;
        return idx;
    } else {
        return -1;
    }
}

void FreeList::free(int idx) {
    if (allocated[idx]) {
        allocated[idx] = false;
        freeList.push_back(idx);
        count--;
    }
}

size_t FreeList::size() {
    return count;
}

size_t FreeList::end() {
    return nextFree;
}

bool FreeList::exists(int idx) {
    return allocated[idx];
}





























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

vec3 spherical2Cartesian(float r, float theta, float phi) {
    float h = r * cosf(phi);
    return { h * cosf(theta), r * sinf(phi), h * sinf(theta) };
}






void trs2Transform(int count, const Trs* trs, mat4* transforms) {
    for (int i = 0; i < count; i++, trs++, transforms++) {
        XMVECTOR q = XMVectorSet(trs->rotation.x, trs->rotation.y, -trs->rotation.z, -trs->rotation.w);
        q = XMQuaternionNormalize(q);
        XMMATRIX S = XMMatrixScaling(trs->scale.x, trs->scale.y, -trs->scale.z);
        XMMATRIX R = XMMatrixRotationQuaternion(q);
        XMMATRIX T = XMMatrixTranslation(trs->position.x, trs->position.y, -trs->position.z);
        XMMATRIX transformMat = S * R * T;
        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(transforms->v), transformMat);
    }
}


}