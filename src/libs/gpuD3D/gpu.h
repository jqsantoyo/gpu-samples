#pragma once
#include <utils/utils.h>
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NOMINMAX
#include <d3d12.h>
#include <d3dcommon.h>
#include <directxmath.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <functional>

#define GUARDHR(x) if (FAILED(x)) {  printf("Error: "#x"\n"); return false; }

namespace gpu::gpu {


D3D12_BLEND_DESC                noBlend();
D3D12_BLEND_DESC                defaultBlend();
D3D12_RENDER_TARGET_BLEND_DESC  noBlendRt();
D3D12_RENDER_TARGET_BLEND_DESC  defaultBlendRt();
D3D12_RASTERIZER_DESC           defaultWire();
D3D12_RASTERIZER_DESC           defaultFill();
D3D12_DEPTH_STENCIL_DESC        noDepthStencil();
D3D12_DEPTH_STENCIL_DESC        defaultDepth();


D3D12_STATIC_SAMPLER_DESC anisotropicSampler();
D3D12_STATIC_SAMPLER_DESC shadowSampler();

D3D12_ROOT_PARAMETER rootCbv(
    UINT shaderRegister,
    UINT registerSpace = 0,
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
);

D3D12_ROOT_PARAMETER rootSrv(
    UINT shaderRegister,
    UINT registerSpace = 0,
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
);

D3D12_ROOT_PARAMETER rootUav(
    UINT shaderRegister,
    UINT registerSpace = 0,
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
);

D3D12_ROOT_PARAMETER rootTable(
    std::initializer_list<D3D12_DESCRIPTOR_RANGE> ranges,
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
);

D3D12_DESCRIPTOR_RANGE rangeSrv(
    UINT numDescriptors,
    UINT baseShaderRegister,
    UINT registerSpace = 0,
    UINT offset = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
);

D3D12_DESCRIPTOR_RANGE rangeUav(
    UINT numDescriptors,
    UINT baseShaderRegister,
    UINT registerSpace = 0,
    UINT offset = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
);

D3D12_DESCRIPTOR_RANGE rangeCbv(
    UINT numDescriptors,
    UINT baseShaderRegister,
    UINT registerSpace = 0,
    UINT offset = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
);


struct Cbv        { int idx; };
struct Srv        { int idx; };
struct Uav        { int idx; };
struct Dsv        { int idx; };
struct Rtv        { int idx; };
struct Queue      { int idx; };
struct Command    { int idx; };
struct Root       { int idx; };
struct Pipeline   { int idx; };
struct Swapchain  { int idx; };
struct Buffer     { int idx; };
struct Texture    { int idx; };

struct PsoGraphicsDesc {
    const char*                             vs;
    const char*                             ps;
    D3D12_STREAM_OUTPUT_DESC                streamOutput;
    D3D12_BLEND_DESC                        blendState;
    UINT                                    sampleMask;
    D3D12_RASTERIZER_DESC                   rasterizerState;
    D3D12_DEPTH_STENCIL_DESC                depthStencilState;
    std::vector<D3D12_INPUT_ELEMENT_DESC>   inputLayout;
    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE      iBStripCutValue;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE           topology;
    UINT                                    numRenderTargets;
    DXGI_FORMAT                             rtvFormats[8];
    DXGI_FORMAT                             dsvFormat;
    DXGI_SAMPLE_DESC                        sampleDesc;
    D3D12_CACHED_PIPELINE_STATE             cachedPSO;
    D3D12_PIPELINE_STATE_FLAGS              flags;
};

struct TextureDesc {
    D3D12_HEAP_TYPE         heap;
    D3D12_RESOURCE_STATES   state;
    uint32_t                width;
    uint32_t                height;
    DXGI_FORMAT             format;
    D3D12_TEXTURE_LAYOUT    layout;
    D3D12_RESOURCE_FLAGS    flags = D3D12_RESOURCE_FLAG_NONE;
    bool                    hasClearValue;
    D3D12_CLEAR_VALUE       clearValue;
};



struct QueueData {
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>  obj;
    Microsoft::WRL::ComPtr<ID3D12Fence>         fence;
    HANDLE                                      fenceEvent;
    UINT64                                      nextFenceValue;
};

struct CommandData {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      cmdAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   cmdList;
    Microsoft::WRL::ComPtr<ID3D12Resource>              constantBuffer;
    uint8_t*                                            data;
    uint32_t                                            maxMemory;
    uint32_t                                            allocatedMemory;
    uint64_t                                            fenceValue;
};

struct RootData {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> obj;
};

struct PipelineData {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> obj;
};

struct SwapTarget {
    Texture texture;
    Rtv     rtv;
};

struct SwapchainData {
    Microsoft::WRL::ComPtr<IDXGISwapChain3> obj;
    std::vector<SwapTarget>                 targets;
    UINT                                    frameIdx;
};

struct BufferData {
    Microsoft::WRL::ComPtr<ID3D12Resource>  obj;
    uint8_t*                                data;
    uint32_t                                size;
    D3D12_HEAP_TYPE                         heap;
    D3D12_RESOURCE_STATES                   state;
    D3D12_RESOURCE_FLAGS                    flags;
};

struct TextureData {
    Microsoft::WRL::ComPtr<ID3D12Resource>  obj;
    D3D12_RESOURCE_STATES                   state;
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

struct Page {
    uint32_t offset;
    uint32_t capacity;
    int head;
};

class Heap {
public:
    bool init(ID3D12Device* gpu, const char* name, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, std::initializer_list<uint32_t> pageCapacities);
    int next(int pageIdx);
    void reset(int pageIdx);
    D3D12_CPU_DESCRIPTOR_HANDLE cpu(UINT idx);
    D3D12_GPU_DESCRIPTOR_HANDLE gpu(UINT idx);
    ID3D12DescriptorHeap* get();
private:
    const char*                                  name;
    UINT                                         size;
    std::vector<Page>                            pages;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> obj;
};


class Gpu {
public:
    void        init            (UINT rtvCount, UINT dsvCount, UINT cbvCount);
    void        terminate       ();
    void        print           ();
    void        printErrors     ();

    Queue       createQueue     (D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
    void        execute         (Queue queue, Command& command);
    void        signal          (Queue queue, Command& command);
    void        wait            (Queue queue, Command& command);
    void        signal          (Queue queue, uint64_t& value);
    void        wait            (Queue queue, uint64_t value);
    void        wait            (Queue queue);

    Command     createCommand   (D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT, uint32_t maxMemory = 0);
    void        begin           (Command command, Pipeline pipeline = { -1 });
    void        viewport        (Command command, D3D12_VIEWPORT& viewport);
    void        scissor         (Command command, D3D12_RECT& rect);
    void        targets         (Command command, Rtv rtv, Dsv dsv);
    void        target          (Command command, Dsv dsv);
    void        clear           (Command command, Rtv rtv, vec4 color);
    void        clear           (Command command, Dsv dsv, float depth, uint8_t stencil);
    void        heaps           (Command command, ID3D12DescriptorHeap* mainHeap, ID3D12DescriptorHeap* samplerHeap = nullptr);
    void        pipeline        (Command command, Pipeline pipeline);
    void        computeRoot     (Command command, Root& root);
    void        computeSrv      (Command command, int idx, Buffer& buffer);
    void        computeUav      (Command command, int idx, Buffer& buffer);
    void        graphicsRoot    (Command command, Root& root);
    void        graphicsCbv     (Command command, int idx, D3D12_GPU_VIRTUAL_ADDRESS addr);
    void        graphicsCbv     (Command command, int idx, void* data, int dataSize);
    template<typename T>    
    void        graphicsCbv     (Command command, int idx, T& t) { graphicsCbv(command, idx, &t, align256(sizeof(T))); }
    void        graphicsTable   (Command command, int idx, D3D12_GPU_DESCRIPTOR_HANDLE descriptor);
    void        barrier         (Command command, Buffer buffer, D3D12_RESOURCE_STATES state);
    void        barrier         (Command command, Texture texture, D3D12_RESOURCE_STATES state);
    void        copy            (Command command, Buffer& dst, Buffer& src);
    void        subresources    (Command command, Texture texture, Buffer uploadBuffer, int offset, int subresourceOffset, const std::vector<D3D12_SUBRESOURCE_DATA>& subresources);
    void        dispatch        (Command command, uint32_t groupsX, uint32_t  groupsY, uint32_t groupsZ);
    void        topology        (Command command, D3D12_PRIMITIVE_TOPOLOGY topology);
    void        vertexBuffer    (Command command, uint32_t slot, D3D12_VERTEX_BUFFER_VIEW view);
    void        indexBuffer     (Command command, D3D12_INDEX_BUFFER_VIEW view);
    void        drawIndexed     (Command command, UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation);
    void        draw            (Command command, UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation);
    void        end             (Command command);

    Root        createRoot      (std::initializer_list<D3D12_ROOT_PARAMETER> params, std::initializer_list<D3D12_STATIC_SAMPLER_DESC> samplers);

    Pipeline    createPipeline  (Root root, const char* shader);
    Pipeline    createPipeline  (Root root, PsoGraphicsDesc desc);

    Swapchain   createSwapchain (Queue& queue, HWND hwnd, uint32_t w, uint32_t h, UINT frameCount);
    void        present         (Swapchain sw, bool vsync);
    SwapTarget  next            (Swapchain sw);

    Buffer      createBuffer    (uint32_t size, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    void        write           (Buffer  buffer, uint32_t offset, uint32_t size, const uint8_t* data);
    void        read            (Buffer  buffer, const std::function<void(uint8_t*)>& f);
    D3D12_GPU_VIRTUAL_ADDRESS addr(Buffer buffer);

    Texture     createTexture   (const uint8_t* data, uint32_t size, std::vector<D3D12_SUBRESOURCE_DATA>& subresources);
    Texture     createTexture   (TextureDesc& desc);


    Cbv         nextCbv         (int page);
    Srv         nextSrv         (int page, Texture texture, D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
    Uav         nextUav         (int page);
    Dsv         nextDsv         (int page, Texture texture, D3D12_DEPTH_STENCIL_VIEW_DESC& desc);
    Rtv         nextRtv         (int page, Texture texture);
    void        resetMain       (int page);
    void        resetDsv        (int page);
    void        resetRtv        (int page);


    ID3D12GraphicsCommandList*  get(Command    command);
    ID3D12RootSignature*        get(Root       root);
    ID3D12PipelineState*        get(Pipeline   pipeline);
    ID3D12Resource*             get(Buffer     buffer);
    ID3D12Resource*             get(Texture    texture);

    Microsoft::WRL::ComPtr<IDXGIFactory4>       factory;
    std::vector<Adapter>                        adapters;
    Adapter*                                    selectedAdapter = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Device>        obj;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue1>    iq;
    Heap                                        mainHeap;
    Heap                                        rtvHeap;
    Heap                                        dsvHeap;
    Vec<Queue,     QueueData>                   queues;
    Vec<Command,   CommandData>                 commands;
    Vec<Root,      RootData>                    roots;
    Vec<Pipeline,  PipelineData>                pipelines;
    Vec<Swapchain, SwapchainData>               swapchains;
    Vec<Buffer,    BufferData>                  buffers;
    Vec<Texture,   TextureData>                 textures;
private:
};









}