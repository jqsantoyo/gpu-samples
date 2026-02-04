#pragma once
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3dx12.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <dxgi1_6.h>
#include <windows.h>

#define GUARD(x)   if (!(x))      {  printf("Error: "#x); return 0; }
#define GUARDHR(x) if (FAILED(x)) {  printf("Error: "#x); return 0; }

namespace gpu {


struct Shader {
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    D3D12_SHADER_BYTECODE bytecode;
};

bool compileShader(Shader& shader, std::wstring dir, std::wstring name, const char* entry, const char* target, uint32_t flags);
bool loadShader(Shader& shader, std::string dir, std::string name);


}