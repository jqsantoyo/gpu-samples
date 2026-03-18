#pragma once
#include <renderer/renderer.h>
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

#define GUARD(x)   if (!(x))      {  printf("Error: "#x"\n"); return 0; }
#define GUARDHR(x) if (FAILED(x)) {  printf("Error: "#x"\n"); return 0; }

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
















class Device {
public:
    bool init(Adapter* adapter, UINT rtvCount, UINT dsvCount, UINT cbvCount);
    void terminate();
    void printErrors();

    Microsoft::WRL::ComPtr<ID3D12Device>            obj;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue>         iq;
    UINT                                            rtvDescSize = 0;
    UINT                                            dsvDescSize = 0;
    UINT                                            cbvDescSize = 0;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    dsvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    cbvHeap;
};











class Queue {
public:
    bool init(Device& device, D3D12_COMMAND_QUEUE_DESC desc);
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


























struct Frame {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  cmdAllocator;
    UINT64                                          fenceValue;
};

class FrameControl {
public:
    bool init(Device& device, Queue* queue, int frameCount);
    bool begin(ID3D12PipelineState* initialState);
    bool execute();
    bool end();
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   cmdList;
private:
    Queue*                                              queue;
    std::vector<Frame>                                  frames;
    int                                                 frameCounter = 0;
    int                                                 frameIdx     = 0;
};












struct Shader {
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    D3D12_SHADER_BYTECODE bytecode;

    bool compile(std::wstring dir, std::wstring name, const char* entry, const char* target, uint32_t flags);
    bool load(std::string name);
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

    
    D3D12_GPU_VIRTUAL_ADDRESS get(int idx) {
        return cb.Get()->GetGPUVirtualAddress() + idx * elementSize;
    }
    
private:
    ID3D12Device* device;
    Microsoft::WRL::ComPtr<ID3D12Resource> cb;
    uint8_t* data;
    uint32_t elementCount;
    uint32_t elementSize;
};













struct Buffer {
    Microsoft::WRL::ComPtr<ID3D12Resource>      vbUp;
};

struct Mesh {
    int                         bufferId;
    size_t                      vCount;
    D3D12_INDEX_BUFFER_VIEW     indicesView;
    D3D12_VERTEX_BUFFER_VIEW    positionView;
    D3D12_VERTEX_BUFFER_VIEW    colorView;
};



class MeshControl {
public:
    bool init();
    int addBuffer(Device& device, const BufferDesc& desc);
    int addMesh(const MeshDesc& desc);
    Mesh& getMesh(int idx);
private:
    std::vector<Buffer> buffers;
    std::vector<Mesh>   meshes;
};





class RootSig {
public:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> obj;
    bool initVoid(Device& device);
    bool init1Cbv(Device& device);
    bool initStd(Device& device);

};



class PipelineBasic {
public:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> obj;
    bool init(Device& device, RootSig& sig);
};

class PipelineFill {
public:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> obj;
    bool init(Device& device, RootSig& sig);
};

class PipelineWire {
public:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> obj;
    bool init(Device& device, RootSig& sig);
};


class DepthBuffer {
public:
    bool init(Device& device, uint64_t width, uint32_t height);
private:
    Microsoft::WRL::ComPtr<ID3D12Resource> obj;
};



}