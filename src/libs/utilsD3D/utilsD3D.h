#pragma once
#include <utils/renderer.h>
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


























struct Frame {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  cmdAllocator;
    UINT64                                          fenceValue;
    Microsoft::WRL::ComPtr<ID3D12Resource>          constantBuffer;
    uint32_t                                        allocatedMemory;
    uint8_t*                                        data;
};

class FrameControl {
public:
    bool init(Device& device, Queue* queue, int frameCount, uint32_t maxMemory = 0);
    bool begin(ID3D12PipelineState* initialState);
    bool execute();
    bool end();
    void barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
    void heaps(ID3D12DescriptorHeap* cbvSrvUavHeap, ID3D12DescriptorHeap* samplerHeap = nullptr);

    template<typename T>
    void setConstantBuffer(UINT idx, T& t) {
        uint32_t dataSize = align256(sizeof(T));
        setConstantBuffer(idx, &t, dataSize);
    }

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   cmdList;
private:
    Queue*                                              queue;
    std::vector<Frame>                                  frames;
    int                                                 frameCounter = 0;
    int                                                 frameIdx     = 0;
    uint32_t                                            maxMemory;
    void setConstantBuffer(UINT idx, void* data, int dataSize);
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













struct Buffer {
    Microsoft::WRL::ComPtr<ID3D12Resource>      vbUp;
};

struct Mesh {
    int                         bufferId;
    size_t                      vCount;
    D3D12_INDEX_BUFFER_VIEW     indicesView;
    D3D12_VERTEX_BUFFER_VIEW    positionView;
    D3D12_VERTEX_BUFFER_VIEW    uvView;
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

struct Texture {
    Microsoft::WRL::ComPtr<ID3D12Resource> res;
    Microsoft::WRL::ComPtr<ID3D12Resource> resUp;
};

class TextureRegistry {
public:
    bool init(Device* device, Queue* queue);
    int addTexture(const char* filename);
    Texture& get(int idx);
private:
    Device* device;
    Queue* queue;
    std::vector<Texture> textures;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  cmdAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   cmdList;
};




class RootSig {
public:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> obj;
    bool initVoid(Device& device);
    bool init1Cbv(Device& device);
    bool init1Cbv1TableNSamplers(Device& device);

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

class PipelineTex {
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