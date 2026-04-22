#pragma once
#include <utils/renderer.h>
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3dx12.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <directxmath.h>
#include <dxgi1_6.h>
#include <windows.h>

#define GUARDHR(x) if (FAILED(x)) {  printf("Error: "#x"\n"); return false; }

namespace gpu {










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
    bool select();
    Adapter* getSelected();
    Microsoft::WRL::ComPtr<IDXGIFactory4>   obj;
    std::vector<Adapter>                    adapters;
    Adapter*                                selectedAdapter = nullptr;
};







class Heap {
public:
    bool init(ID3D12Device* device, const char* name, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT capacity);
    void reset();
    bool next(int& idx);
    D3D12_CPU_DESCRIPTOR_HANDLE getCpu(UINT idx);
    D3D12_GPU_DESCRIPTOR_HANDLE getGpu(UINT idx);
    ID3D12DescriptorHeap* get();
private:
    const char*                                  name;
    UINT                                         size       = 0;
    UINT                                         capacity   = 0;
    int                                          currentIdx = 0;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> obj;
};


class Device {
public:
    bool init(Adapter* adapter, UINT rtvCount, UINT dsvCount, UINT cbvCount);
    void terminate();
    void reset();
    void printErrors();


    Microsoft::WRL::ComPtr<ID3D12Device>    obj;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> iq;
    Heap                                    rtvHeap;
    Heap                                    dsvHeap;
    Heap                                    cbvHeap;
};







class Queue {
public:
    bool init(
        Device& device,
        D3D12_COMMAND_LIST_TYPE type,
        INT priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        UINT nodeMask = 0
    );
    void terminate();
    void execute(std::initializer_list<ID3D12CommandList*> lists);
    bool signal(UINT64& value);
    bool wait(UINT64 value);
    bool wait();
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>  obj;
private:
    Microsoft::WRL::ComPtr<ID3D12Fence>         fence;
    HANDLE                                      fenceEvent;
    UINT64                                      nextFenceValue;
};








struct RenderTarget {
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    CD3DX12_CPU_DESCRIPTOR_HANDLE view;
};

class Swapchain {
public:
    bool init(Factory& factory, Device& device, Queue& queue, HWND hwnd, uint32_t w, uint32_t h, UINT frameCount);
    RenderTarget next();
    bool present(bool vsync);

// private:
    Microsoft::WRL::ComPtr<IDXGISwapChain3> obj;
    std::vector<RenderTarget>               renderTargets;
    UINT                                    frameIdx;
};







struct Shader {
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    D3D12_SHADER_BYTECODE bytecode;

    bool compile(std::wstring dir, std::wstring name, const char* entry, const char* target, uint32_t flags);
    bool load(std::string name);
};







// Type unsafe Constant Buffer class, use derived ConstantBuffer.
class ConstantBufferBase {
public:
    ~ConstantBufferBase();
    bool init(ID3D12Device* device, uint32_t elementCount, uint32_t elementSize);
    ID3D12Resource* getRes();
    void set(int idx, void* element);
    D3D12_GPU_VIRTUAL_ADDRESS getAddress(int idx);
    D3D12_CONSTANT_BUFFER_VIEW_DESC getView(int idx);
    
private:
    ID3D12Device* device;
    Microsoft::WRL::ComPtr<ID3D12Resource> obj;
    uint8_t* data;
    uint32_t elementCount;
    uint32_t elementSize;
    uint32_t elementSizePadded;
    
};



template<typename T>
class ConstantBuffer : public ConstantBufferBase {
public:

    bool init(ID3D12Device* device, uint32_t elementCount) {
        return ConstantBufferBase::init(device, elementCount, sizeof(T));
    }

    void set(int idx, T& element) {
        ConstantBufferBase::set(idx, &element);
    }
};




}