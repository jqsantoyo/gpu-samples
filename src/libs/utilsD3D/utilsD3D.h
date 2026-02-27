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

    bool compile(std::wstring dir, std::wstring name, const char* entry, const char* target, uint32_t flags);
    bool load(std::string dir, std::string name);
};



struct Output {
    IDXGIOutput*     obj;
    DXGI_OUTPUT_DESC desc;
    std::vector<DXGI_MODE_DESC> modes;
};

struct Adapter {
    IDXGIAdapter*       obj;
    DXGI_ADAPTER_DESC   desc;
    std::vector<Output> outputs;
};

class Factory {
public:
    bool init();
    void print();
    Microsoft::WRL::ComPtr<IDXGIFactory4>   factory;
    std::vector<Adapter>                    adapters;
};



template<typename T>
class CBuffer {
public:
    CBuffer() {};

    ~CBuffer() {
        if (cb != nullptr) {
            cb->Unmap(0, nullptr);
        }
        data = nullptr;
    }

    void init(ID3D12Device* device, uint32_t elementCount) {
        this->device = device;
        this->elementCount = elementCount;
        this->elementSize = align256(sizeof(T));
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(elementSize * elementCount);
        HRESULT res = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&cb)
        );
        cb->Map(0, nullptr, reinterpret_cast<void**>(&data));
        this->device = device;
    }

    void set(int idx, T& element) {
        memcpy(data + elementSize * idx, &element, sizeof(T));
    }

    D3D12_CONSTANT_BUFFER_VIEW_DESC getBufferViewDesc(int idx) {
        return {
            .BufferLocation = cb->GetGPUVirtualAddress() + idx * elementSize,
            .SizeInBytes = elementSize,
        };
    }

    ID3D12Resource* get() {
        return cb.Get();
    }
    
private:
    ID3D12Device* device;
    Microsoft::WRL::ComPtr<ID3D12Resource> cb;
    uint8_t* data;
    uint32_t elementCount;
    uint32_t elementSize;
};




}