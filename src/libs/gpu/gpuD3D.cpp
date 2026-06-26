#include "gpu.h"
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NOMINMAX
#include <ddsTextureLoader/DDSTextureLoader12.h>
#include <d3dx12.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <directxmath.h>
#include <dxgi1_6.h>
#include <pix3.h>
#include <wrl/client.h>
#include <cstdarg>
using namespace DirectX;
using namespace Microsoft::WRL;

#define ASSERT_DX(x) { assert(SUCCEEDED(x)); }

namespace gpu {

////////////////////////////////////////////////////////////////////////////////////////////////////
/// UTILITIES //////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

struct Cbv        { int idx; };
struct Srv        { int idx; };
struct Uav        { int idx; };
struct Dsv        { int idx; };
struct Rtv        { int idx; };

class Heap {
public:
    struct AllocResult {
        int idx;
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
    };

    void init(ID3D12Device* device, const char* name, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible, uint32_t capacity) {
        this->name = name;
        if (capacity > 0) {
            descriptorSize = device->GetDescriptorHandleIncrementSize(type);
            freeList.init(capacity);
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type           = type,
                .NumDescriptors = capacity,
                .Flags          = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
                .NodeMask       = 0,
            };
            ASSERT_DX(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&obj)));
        }
    }

    AllocResult alloc() {
        int idx = freeList.alloc();
        ASSERT(idx != -1);
        return { idx, cpu(idx) };
    }

    void free(int idx) {
        freeList.free(idx);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu(UINT idx) {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(obj->GetCPUDescriptorHandleForHeapStart(), idx, descriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE gpu(UINT idx) {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(obj->GetGPUDescriptorHandleForHeapStart(), idx, descriptorSize);
    }

    ID3D12DescriptorHeap* get() {
        return obj.Get();
    }

    const char*                                  name;
    uint32_t                                     descriptorSize;
    FreeList                                     freeList;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> obj;
};



D3D12_COMMAND_LIST_TYPE nativeQueueType(QueueType type) {
    switch(type) {
        case QueueGraphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case  QueueCompute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case     QueueCopy: return D3D12_COMMAND_LIST_TYPE_COPY;
        default           : return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }
}

DXGI_FORMAT getNativeFormat(Format format) {
    switch(format) {
        case Format::Unknown       : return DXGI_FORMAT_UNKNOWN;

        case Format::D16           : return DXGI_FORMAT_D16_UNORM;
        case Format::D24S8         : return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case Format::D32           : return DXGI_FORMAT_D32_FLOAT;
        case Format::D32S8         : return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        
        case Format::R16f          : return DXGI_FORMAT_R16_FLOAT;
        case Format::RG16f         : return DXGI_FORMAT_R16G16_FLOAT;
        case Format::RGBA16f       : return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case Format::R32f          : return DXGI_FORMAT_R32_FLOAT;
        case Format::RG32f         : return DXGI_FORMAT_R32G32_FLOAT;
        case Format::RGB32f        : return DXGI_FORMAT_R32G32B32_FLOAT;
        case Format::RGBA32f       : return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::R11G11B10f    : return DXGI_FORMAT_R11G11B10_FLOAT;
        
        case Format::R8u           : return DXGI_FORMAT_R8_UINT;
        case Format::RG8u          : return DXGI_FORMAT_R8G8_UINT;
        case Format::RGBA8u        : return DXGI_FORMAT_R8G8B8A8_UINT;
        case Format::R16u          : return DXGI_FORMAT_R16_UINT;
        case Format::RG16u         : return DXGI_FORMAT_R16G16_UINT;
        case Format::RGBA16u       : return DXGI_FORMAT_R16G16B16A16_UINT;
        case Format::R32u          : return DXGI_FORMAT_R32_UINT;
        case Format::RG32u         : return DXGI_FORMAT_R32G32_UINT;
        case Format::RGB32u        : return DXGI_FORMAT_R32G32B32_UINT;
        case Format::RGBA32u       : return DXGI_FORMAT_R32G32B32A32_UINT;
        case Format::R10G10B10A2u  : return DXGI_FORMAT_R10G10B10A2_UINT;
        
        case Format::R8s           : return DXGI_FORMAT_R8_SINT;
        case Format::RG8s          : return DXGI_FORMAT_R8G8_SINT;
        case Format::RGBA8s        : return DXGI_FORMAT_R8G8B8A8_SINT;
        case Format::R16s          : return DXGI_FORMAT_R16_SINT;
        case Format::RG16s         : return DXGI_FORMAT_R16G16_SINT;
        case Format::RGBA16s       : return DXGI_FORMAT_R16G16B16A16_SINT;
        case Format::R32s          : return DXGI_FORMAT_R32_SINT;
        case Format::RG32s         : return DXGI_FORMAT_R32G32_SINT;
        case Format::RGB32s        : return DXGI_FORMAT_R32G32B32_SINT;
        case Format::RGBA32s       : return DXGI_FORMAT_R32G32B32A32_SINT;
        
        case Format::R8un          : return DXGI_FORMAT_R8_UNORM;
        case Format::RG8un         : return DXGI_FORMAT_R8G8_UNORM;
        case Format::RGBA8un       : return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::BGRA8un       : return DXGI_FORMAT_B8G8R8A8_UNORM;
        case Format::RGBA8un_sRGB  : return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case Format::BGRA8un_sRGB  : return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case Format::R16un         : return DXGI_FORMAT_R16_UNORM;
        case Format::RG16un        : return DXGI_FORMAT_R16G16_UNORM;
        case Format::RGBA16un      : return DXGI_FORMAT_R16G16B16A16_UNORM;
        case Format::R10G10B10A2un : return DXGI_FORMAT_R10G10B10A2_UNORM;
        
        case Format::R8sn          : return DXGI_FORMAT_R8_SNORM;
        case Format::RG8sn         : return DXGI_FORMAT_R8G8_SNORM;
        case Format::RGBA8sn       : return DXGI_FORMAT_R8G8B8A8_SNORM;
        case Format::R16sn         : return DXGI_FORMAT_R16_SNORM;
        case Format::RG16sn        : return DXGI_FORMAT_R16G16_SNORM;
        case Format::RGBA16sn      : return DXGI_FORMAT_R16G16B16A16_SNORM;

        default                    : return DXGI_FORMAT_UNKNOWN;
    }
}

D3D12_RESOURCE_STATES getNativeState(State state) {
    switch(state) {
        case State::Common:             return D3D12_RESOURCE_STATE_COMMON;
        case State::RenderTarget:       return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case State::UnorderedAccess:    return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case State::DepthWrite:         return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case State::DepthRead:          return D3D12_RESOURCE_STATE_DEPTH_READ;
        case State::PSRead:             return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case State::CopyDest:           return D3D12_RESOURCE_STATE_COPY_DEST;
        case State::CopySource:         return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case State::GenericRead:        return D3D12_RESOURCE_STATE_GENERIC_READ;
        case State::Present:            return D3D12_RESOURCE_STATE_PRESENT;
    }
}

D3D12_FILL_MODE getNativeFillMode(FillMode mode) {
    switch(mode) {
        case FillMode::Wireframe : return D3D12_FILL_MODE_WIREFRAME;
        case FillMode::Solid     : return D3D12_FILL_MODE_SOLID;
    }
}

D3D12_CULL_MODE getNativeCullMode(CullMode mode) {
    switch(mode) {
        case CullMode::None  : return D3D12_CULL_MODE_NONE;
        case CullMode::Front : return D3D12_CULL_MODE_FRONT;
        case CullMode::Back  : return D3D12_CULL_MODE_BACK;
    }
}

D3D_PRIMITIVE_TOPOLOGY getNativePrimitiveTopology(PrimitiveTopology primitiveTopology) {
    switch(primitiveTopology) {
        case PrimitiveTopology::Undefined       : return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        case PrimitiveTopology::PointList       : return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::LineList        : return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::LineStrip       : return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveTopology::TriangleList    : return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip   : return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case PrimitiveTopology::TriangleFan     : return D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN;
        case PrimitiveTopology::LineListAdj     : return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
        case PrimitiveTopology::LineStripAdj    : return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
        case PrimitiveTopology::TriangleListAdj : return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
        case PrimitiveTopology::TriangleStripAdj: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
    }
};


D3D12_ROOT_PARAMETER_TYPE getNativeBindingParamType(RootBinding binding) {
    switch(binding) {
        case RootBinding::Constant : return D3D12_ROOT_PARAMETER_TYPE_CBV;
        case RootBinding::Read     : return D3D12_ROOT_PARAMETER_TYPE_SRV;
        case RootBinding::ReadWrite: return D3D12_ROOT_PARAMETER_TYPE_UAV;
    }
}

D3D12_DESCRIPTOR_RANGE_TYPE getNativeRangeType(RootBinding rangeBinding) {
    switch(rangeBinding) {
        case RootBinding::Constant : return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        case RootBinding::Read     : return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        case RootBinding::ReadWrite: return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    }
}

D3D12_FILTER getNativeFilter(Filter filter) {
    switch(filter) {
        case Filter::Point      : return D3D12_FILTER_MIN_MAG_MIP_POINT;
        case Filter::Linear     : return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        case Filter::Anisotropic: return D3D12_FILTER_ANISOTROPIC;
    }
}

D3D12_TEXTURE_ADDRESS_MODE getNativeAddressMode(AddressMode mode) {
    switch(mode) {
        case AddressMode::Wrap       : return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case AddressMode::Mirror     : return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case AddressMode::Clamp      : return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case AddressMode::Border     : return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case AddressMode::Mirror_once: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
    }
}

D3D12_STATIC_BORDER_COLOR getNativeSamplerBorderColor(SamplerBorderColor color) {
    switch(color) {
        case SamplerBorderColor::TransparentBlack : return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        case SamplerBorderColor::OpaqueBlack     : return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        case SamplerBorderColor::OpaqueWhite: return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    }
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE getNativePrimitiveTopologyType(PrimitiveTopologyType type) {
    switch(type) {
        case PrimitiveTopologyType::Undefined: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        case PrimitiveTopologyType::Point:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case PrimitiveTopologyType::Line:      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PrimitiveTopologyType::Triangle:  return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case PrimitiveTopologyType::Patch:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }
}

void setResourceName(ID3D12Resource* resource, const char* name) {
    int length = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
    std::wstring wName(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wName.data(), length);
    resource->SetName(wName.c_str());
}







































////////////////////////////////////////////////////////////////////////////////////////////////////
/// INTERFACE //////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class Shader {
public:
    Shader(std::string name) {
        std::string path = name + ".dxil";
        ASSERT(readFile(path.c_str(), data));
    }
    
    D3D12_SHADER_BYTECODE bytecode() {
        return { data.data(), data.size() };
    }

private:
    std::vector<uint8_t> data;
};

RootRange RootRange::range(RootBinding binding, uint32_t bindingCount, uint32_t baseShaderRegister, uint32_t space) {
    return RootRange {
        .binding            = binding,
        .bindingCount       = bindingCount,
        .baseShaderRegister = baseShaderRegister,
        .registerSpace      = space,
    };
}

RootParam RootParam::binding(RootBinding binding, uint32_t shaderRegister, uint32_t space) {
    return RootParam {
        .paramType      = RootParamType::Binding,
        .bindingType    = binding,
        .shaderRegister = shaderRegister,
        .registerSpace  = space,
    };
}

RootParam RootParam::data(uint32_t count, uint32_t shaderRegister, uint32_t space) {
    return RootParam {
        .paramType      = RootParamType::Data,
        .dataCount      = count,
        .shaderRegister = shaderRegister,
        .registerSpace  = space,
    };
}

RootParam RootParam::table(std::initializer_list<RootRange> ranges) {
    RootParam rootParam = {};
    rootParam.paramType = RootParamType::Table;
    rootParam.tableRanges.assign(ranges.begin(), ranges.end());
    return rootParam;
}










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


struct SwapchainData {
    Microsoft::WRL::ComPtr<IDXGISwapChain3> obj;
    std::vector<SwapTarget>                 targets;
    UINT                                    frameIdx;
};

struct BufferData {
    Microsoft::WRL::ComPtr<ID3D12Resource>  obj;
    uint8_t*                                data;
    uint32_t                                size;
    BufferUsage                             usage;
    D3D12_HEAP_TYPE                         heap;
    D3D12_RESOURCE_STATES                   state;
    D3D12_RESOURCE_FLAGS                    flags;
};

struct TextureData {
    Microsoft::WRL::ComPtr<ID3D12Resource>  obj;
    D3D12_RESOURCE_STATES                   state;
};

enum TextureViewAspect {
    TextureViewColor,
    TextureViewDepth,
    TextureViewRender
};

struct TextureViewData {
    TextureViewAspect aspect;
    int               idx;
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




void deviceMessage( 
    D3D12_MESSAGE_CATEGORY Category,
    D3D12_MESSAGE_SEVERITY Severity,
    D3D12_MESSAGE_ID ID,
    LPCSTR pDescription,
    void *pContext
) {
    printf("Gpu message\n");
}







class GpuD3D : public IGpu {
public:

GpuD3D(const GpuDesc& desc) {
    // Factory & adapter list initialization
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    ASSERT_DX(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));

    int i = 0;
    IDXGIAdapter* adapter = nullptr;
    while(factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
        adapters.push_back({});
        Adapter& a = adapters.back();

        a.obj = adapter;
        adapter->GetDesc(&a.desc);
        
        int j = 0;
        IDXGIOutput* output = nullptr;
        while(adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND) {
            a.outputs.push_back({});
            Output& o = a.outputs.back();

            o.obj = output;
            output->GetDesc(&o.desc);
            UINT modeCount = 0;
            output->GetDisplayModeList(format, 0, &modeCount, nullptr);
            o.modes.resize(modeCount);
            output->GetDisplayModeList(format, 0, &modeCount, &o.modes[0]);
            j++;
        }
        i++;
    }

    // Adapter selection
    size_t maxMemory = 0;
    for (auto& a : adapters) {
        if (a.desc.DedicatedVideoMemory > maxMemory) {
            maxMemory = a.desc.DedicatedVideoMemory;
            selectedAdapter = &a;
        }
    }
    ASSERT(selectedAdapter != nullptr);


    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        printf("Debug layer enabled\n");
        debugController->EnableDebugLayer();
    }
    // ComPtr<ID3D12Debug1> debugController1;
    // if (SUCCEEDED(debugController.As(&debugController1))) {
    //     debugController1->SetEnableGPUBasedValidation(TRUE);
    // }
    ASSERT_DX(D3D12CreateDevice(selectedAdapter->obj, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&obj)));
    if (SUCCEEDED(obj.As(&iq))) {
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,        FALSE);
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,      FALSE);
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION,   FALSE);
    }
    iq->RegisterMessageCallback(deviceMessage, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, nullptr);

    
    colorViewCount   = 0;
    depthViewCount   = 0;
    renderViewCount  = 0;
    maxColorViews    = desc.maxColorViews;
    maxDepthViews    = desc.maxDepthViews;
    maxRenderViews   = desc.maxRenderViews;
    maxTextureViews  = maxColorViews + maxRenderViews + maxDepthViews;
    textureViews.init(maxTextureViews);

    queues      .init(desc.queueCount);
    commands    .init(desc.commandCount);
    roots       .init(desc.rootCount);
    pipelines   .init(desc.pipelineCount);
    swapchains  .init(desc.swapchainCount);
    buffers     .init(desc.bufferCount);
    textures    .init(desc.textureCount);

    mainHeap    .init(obj.Get(), "mainHeap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true,  desc.maxColorViews);
    dsvHeap     .init(obj.Get(), "dsvHeap",  D3D12_DESCRIPTOR_HEAP_TYPE_DSV,         false, desc.maxDepthViews);
    rtvHeap     .init(obj.Get(), "rtvHeap",  D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         false, desc.maxRenderViews);
    
    // D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels = {
    //     .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
    //     .SampleCount = 4,
    //     .Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
    //     .NumQualityLevels = 0,
    // };
    // GUARDHR(gpu->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)));
    // GUARD((qualityLevels.NumQualityLevels > 0));
    // DXGI_SAMPLE_DESC sampleDesc = { .Count = 4, .Quality = qualityLevels.NumQualityLevels - 1 }; 
}

~GpuD3D() {
    terminate();
}

void terminate() {

}






void print() {
    printf("\nAdapters: %zu\n", adapters.size());
    for (auto& a : adapters) {
        printf("Adapter\n");
        printf("  Description: %ls\n",              a.desc.Description);
        printf("  VendorId: %d\n",                  a.desc.VendorId);
        printf("  DeviceId: %d\n",                  a.desc.DeviceId);
        printf("  SubSysId: %d\n",                  a.desc.SubSysId);
        printf("  Revision: %d\n",                  a.desc.Revision);
        printf("  DedicatedVideoMemory: %zu\n",     a.desc.DedicatedVideoMemory);
        printf("  DedicatedSystemMemory: %zu\n",    a.desc.DedicatedSystemMemory);
        printf("  SharedSystemMemory: %zu\n",       a.desc.SharedSystemMemory);
        for (auto& o : a.outputs) {
            printf("    Output: %ls\n", o.desc.DeviceName);
            for (auto m : o.modes) {
                printf("      resolution: %d x %d, refresh: %d / %d\n", m.Width, m.Height, m.RefreshRate.Numerator, m.RefreshRate.Denominator);
            }
        }
    }
}


void printErrors() {
    UINT64 count = iq->GetNumStoredMessages();
    if (count > 0) {
        // printf("D3D12: messages in queue = %llu\n", count);
        for (UINT64 i = 0; i < count; i++) {
            SIZE_T size = 0;
            iq->GetMessage(i, nullptr, &size);
            auto* msg = (D3D12_MESSAGE*)malloc(size);
            iq->GetMessage(i, msg, &size);
            printf("D3D12 MSG[%llu/%llu]: %s\n", i, count, msg->pDescription);
            free(msg);
        }
        __debugbreak();
    }
}


















Queue createQueue(QueueType type) {
    Queue queue = queues.alloc();
    QueueData& queueData = queues[queue];
    D3D12_COMMAND_LIST_TYPE natType = nativeQueueType(type);
    D3D12_COMMAND_QUEUE_DESC desc = {
        .Type       = natType,
        .Priority   = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags      = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask   = 0
    };
    ASSERT_DX(obj->CreateCommandQueue(&desc, IID_PPV_ARGS(&queueData.obj)));
    ASSERT_DX(obj->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&queueData.fence)));
    queueData.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (queueData.fenceEvent == nullptr) {
        ASSERT_DX(HRESULT_FROM_WIN32(GetLastError()));
    }
    return queue;
}

void execute(Queue queue, Command& command) {
    QueueData& queueData = queues[queue];
    CommandData& commandData = commands[command];
    ID3D12CommandList* cmdLists[] = { commandData.cmdList.Get() };
    queueData.obj->ExecuteCommandLists(1, cmdLists);
}

void signal(Queue queue, Command& command) {
    QueueData& queueData = queues[queue];
    CommandData& commandData = commands[command];
    signal(queue, commandData.fenceValue);
}

void wait(Queue queue, Command& command) {
    QueueData& queueData = queues[queue];
    CommandData& commandData = commands[command];
    wait(queue, commandData.fenceValue);
}

void signal(Queue queue, uint64_t& value) {
    QueueData& queueData = queues[queue];
    queueData.nextFenceValue++;
    value = queueData.nextFenceValue;
    ASSERT_DX(queueData.obj->Signal(queueData.fence.Get(), queueData.nextFenceValue));

}

void wait(Queue queue, uint64_t value) {
    QueueData& queueData = queues[queue];
    if (queueData.fence->GetCompletedValue() < value) {
        ASSERT_DX(queueData.fence->SetEventOnCompletion(value, queueData.fenceEvent));
        WaitForSingleObject(queueData.fenceEvent, INFINITE);
    }
}


void wait(Queue queue) {
    UINT64 value;
    signal(queue, value);
    wait(queue, value);
}
































Command createCommand(QueueType type, uint32_t maxMemory) {
    Command command = commands.alloc();
    CommandData& commandData = commands[command];
    D3D12_COMMAND_LIST_TYPE natType = nativeQueueType(type);
    ASSERT_DX(obj->CreateCommandAllocator(natType, IID_PPV_ARGS(&commandData.cmdAllocator)));
    ASSERT_DX(obj->CreateCommandList(0, natType, commandData.cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&commandData.cmdList)));
    ASSERT_DX(commandData.cmdList.Get()->Close());

    commandData.fenceValue = 0;
    commandData.allocatedMemory = 0;
    commandData.maxMemory = align256(maxMemory);
    if (maxMemory != 0) {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(commandData.maxMemory);
        HRESULT res = obj->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&commandData.constantBuffer)
        );
        commandData.constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&commandData.data));
    } else {
        commandData.data = nullptr;
    }
    return command;
}

void begin(Command command, Pipeline pipeline) {
    CommandData& commandData = commands[command];
    ID3D12PipelineState* initialState = pipeline.idx != -1 ? pipelines[pipeline].obj.Get() : nullptr;
    ASSERT_DX(commandData.cmdAllocator->Reset());
    ASSERT_DX(commandData.cmdList->Reset(commandData.cmdAllocator.Get(), initialState));
    commandData.allocatedMemory = 0;
}

void bindHeap(Command command) {
    CommandData& commandData = commands[command];
    ID3D12DescriptorHeap* heaps[] = { mainHeap.get(), nullptr };
    commandData.cmdList->SetDescriptorHeaps(1, heaps);
}

void viewport(Command command, Viewport& viewport) {
    CommandData& commandData = commands[command];
    D3D12_VIEWPORT nativeViewport = {
        .TopLeftX   = viewport.topLeftX,
        .TopLeftY   = viewport.topLeftY,
        .Width      = viewport.width,
        .Height     = viewport.height,
        .MinDepth   = viewport.minDepth,
        .MaxDepth   = viewport.maxDepth,
    };
    commandData.cmdList->RSSetViewports(1, &nativeViewport);
}

void scissor(Command command, Rect& rect) {
    CommandData& commandData = commands[command];
    D3D12_RECT nativeRect = {
        .left   = static_cast<LONG>(rect.left),
        .top    = static_cast<LONG>(rect.top),
        .right  = static_cast<LONG>(rect.right),
        .bottom = static_cast<LONG>(rect.bottom),
    };
    commandData.cmdList->RSSetScissorRects(1, &nativeRect);
}

void targets(Command command, TextureView renderTexture, TextureView depthTexture) {
    CommandData& commandData = commands[command];
    int numRenderTargets;
    D3D12_CPU_DESCRIPTOR_HANDLE  rtvDes;
    D3D12_CPU_DESCRIPTOR_HANDLE* rtvPtr;
    D3D12_CPU_DESCRIPTOR_HANDLE  dsvDes;
    D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr;
    if (renderTexture.idx != -1) {
        rtvDes = renderViewCpu(renderTexture);
        rtvPtr = &rtvDes;
        numRenderTargets = 1;
    } else {
        rtvPtr = nullptr;
        numRenderTargets = 0;
    }
    if (depthTexture.idx != -1) {
        dsvDes = depthViewCpu(depthTexture);
        dsvPtr = &dsvDes;
    } else {
        dsvPtr = nullptr;
    }
    commandData.cmdList->OMSetRenderTargets(numRenderTargets, rtvPtr, false, dsvPtr);
}

void clear(Command command, TextureView renderTexture, vec4 color, TextureView depthTexture, float depth, uint8_t stencil) {
    CommandData& commandData = commands[command];
    if (renderTexture.idx != -1) {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvDes = renderViewCpu(renderTexture);
        commandData.cmdList->ClearRenderTargetView(rtvDes, &color.x, 0, nullptr);
    }
    if (depthTexture.idx != -1) {
        D3D12_CPU_DESCRIPTOR_HANDLE dsvDes = depthViewCpu(depthTexture);
        commandData.cmdList->ClearDepthStencilView(dsvDes, D3D12_CLEAR_FLAG_DEPTH, depth, stencil, 0, nullptr);
    }
}

void computeRoot(Command command, Root& root) {
    CommandData& commandData = commands[command];
    RootData& rootData = roots[root];
    commandData.cmdList->SetComputeRootSignature(rootData.obj.Get());
}

void computeSrv(Command command, int idx, Buffer& buffer) {
    CommandData& commandData = commands[command];
    BufferData& bufferData = buffers[buffer];
    commandData.cmdList->SetComputeRootShaderResourceView(idx, bufferData.obj->GetGPUVirtualAddress());
}

void computeUav(Command command, int idx, Buffer& buffer) {
    CommandData& commandData = commands[command];
    BufferData& bufferData = buffers[buffer];
    commandData.cmdList->SetComputeRootUnorderedAccessView(idx, bufferData.obj->GetGPUVirtualAddress());
}

void dispatch(Command command, uint32_t groupsX, uint32_t  groupsY, uint32_t groupsZ) {
    CommandData& commandData = commands[command];
    commandData.cmdList->Dispatch(groupsX, groupsY, groupsZ);
}

void graphicsRoot(Command command, Root& root) {
    CommandData& commandData = commands[command];
    RootData& rootData = roots[root];
    commandData.cmdList->SetGraphicsRootSignature(rootData.obj.Get());
}

void graphicsCbv(Command command, int idx, D3D12_GPU_VIRTUAL_ADDRESS addr) {
    CommandData& commandData = commands[command];
    commandData.cmdList->SetGraphicsRootConstantBufferView(idx, addr);
}

void graphicsCbv(Command command, int idx, const void* data, int dataSize) {
    CommandData& commandData = commands[command];
    memcpy(commandData.data + commandData.allocatedMemory, data, dataSize);
    D3D12_GPU_VIRTUAL_ADDRESS addr = commandData.constantBuffer->GetGPUVirtualAddress() + commandData.allocatedMemory;
    commandData.cmdList->SetGraphicsRootConstantBufferView(idx, addr);
    commandData.allocatedMemory += dataSize;
}

void graphicsTable(Command command, int idx, int descriptorIdx) {
    CommandData& commandData = commands[command];
    D3D12_GPU_DESCRIPTOR_HANDLE descriptor = mainHeap.gpu(descriptorIdx);
    commandData.cmdList->SetGraphicsRootDescriptorTable(idx, descriptor);
}

void barrier(Command command, Buffer buffer, State state) {
    CommandData& commandData = commands[command];
    BufferData& bufferData = buffers[buffer];
    D3D12_RESOURCE_STATES nativeState = getNativeState(state);
    if (bufferData.state != nativeState) {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(get(buffer), bufferData.state, nativeState);
        bufferData.state = nativeState;
        commandData.cmdList->ResourceBarrier(1, &barrier);
    }
}

void barrier(Command command, Texture texture, State state) {
    CommandData& commandData = commands[command];
    TextureData& textureData = textures[texture];
    D3D12_RESOURCE_STATES nativeState = getNativeState(state);
    if (textureData.state != nativeState) {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(get(texture), textureData.state, nativeState);
        textureData.state = nativeState;
        commandData.cmdList->ResourceBarrier(1, &barrier);
    }
}

void copy(Command command, Buffer& dst, Buffer& src) {
    CommandData& commandData = commands[command];
    ID3D12Resource* dstRes = get(dst);
    ID3D12Resource* srcRes = get(src);
    auto barr0 = CD3DX12_RESOURCE_BARRIER::Transition(srcRes, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto barr1 = CD3DX12_RESOURCE_BARRIER::Transition(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    commandData.cmdList->ResourceBarrier(1, &barr0);
    commandData.cmdList->CopyResource(dstRes, srcRes);
    commandData.cmdList->ResourceBarrier(1, &barr1);
}

void subresources(Command command, Texture texture, Buffer uploadBuffer, int offset, int subresourceOffset, const std::vector<ResourceData>& subresources) {
    std::vector<D3D12_SUBRESOURCE_DATA> nativeSubresources(subresources.size());
    for (int i = 0; i < nativeSubresources.size(); i++) {
        const ResourceData& sub = subresources[i];
        nativeSubresources[i] = { sub.data, sub.rowPitch, sub.slicePitch };
    }
    UpdateSubresources(get(command), get(texture), get(uploadBuffer), offset, subresourceOffset, nativeSubresources.size(), nativeSubresources.data());
}

void pipeline(Command command, Pipeline pipeline) {
    CommandData& commandData = commands[command];
    PipelineData& pipelineData = pipelines[pipeline];
    commandData.cmdList->SetPipelineState(pipelineData.obj.Get());
}

void topology(Command command, PrimitiveTopology primitiveTopology) {
    CommandData& commandData = commands[command];
    D3D12_PRIMITIVE_TOPOLOGY nativeTopo = getNativePrimitiveTopology(primitiveTopology);
    commandData.cmdList->IASetPrimitiveTopology(nativeTopo);
}

void vertexBuffer(Command command, uint32_t slot, VertexBufferView view) {
    CommandData& commandData = commands[command];
    D3D12_VERTEX_BUFFER_VIEW nativeView = {
        .BufferLocation = addr(view.buffer) + view.offset,
        .SizeInBytes    = view.size,
        .StrideInBytes  = view.stride,
    };
    commandData.cmdList->IASetVertexBuffers(slot, 1, &nativeView);
}
void indexBuffer(Command command, IndexBufferView view) {
    CommandData& commandData = commands[command];
    D3D12_INDEX_BUFFER_VIEW nativeView = {
        .BufferLocation = addr(view.buffer) + view.offset,
        .SizeInBytes    = view.size,
        .Format         = getNativeFormat(view.format),
    };
    commandData.cmdList->IASetIndexBuffer(&nativeView);
}

void drawIndexed(Command command, uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int baseVertexLocation, uint32_t startInstanceLocation) {
    CommandData& commandData = commands[command];
    commandData.cmdList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void draw(Command command, uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) {
    CommandData& commandData = commands[command];
    commandData.cmdList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void end(Command command) {
    CommandData& commandData = commands[command];
    ASSERT_DX(commandData.cmdList->Close());
}
















Root createRoot(const std::vector<RootParam>& params, const std::vector<Sampler>& samplers) {
    Root root = roots.alloc();
    RootData& rootData = roots[root];
    ComPtr<ID3DBlob>            sig;
    ComPtr<ID3DBlob>            error;

    std::vector<D3D12_ROOT_PARAMETER>                nativeParams(params.size());
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> nativeTables(params.size()); // possibly less tables than params but it doesn't hurt
    std::vector<D3D12_STATIC_SAMPLER_DESC>           nativeSamplers(samplers.size());

    for (int i = 0; i < params.size(); i++) {
        const RootParam& rootParam = params[i];
        switch(rootParam.paramType) {
        case RootParamType::Binding:
            nativeParams[i] = {
                .ParameterType = getNativeBindingParamType(rootParam.bindingType),
                .Descriptor = {
                    .ShaderRegister = rootParam.shaderRegister,
                    .RegisterSpace  = rootParam.registerSpace,
                },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            };
        break;
        case RootParamType::Data:
            nativeParams[i] = {
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                .Constants = {
                    .ShaderRegister = rootParam.shaderRegister,
                    .RegisterSpace  = rootParam.registerSpace,
                    .Num32BitValues = rootParam.dataCount,
                },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            };

        break;
        case RootParamType::Table:
            nativeTables[i].resize(rootParam.tableRanges.size());
            for (int j = 0; j < rootParam.tableRanges.size(); j++) {
                const RootRange& range = rootParam.tableRanges[j];
                nativeTables[i][j] = {
                    .RangeType                          = getNativeRangeType(range.binding),
                    .NumDescriptors                     = range.bindingCount,
                    .BaseShaderRegister                 = range.baseShaderRegister,
                    .RegisterSpace                      = range.registerSpace,
                    .OffsetInDescriptorsFromTableStart  = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
                };
            }
            nativeParams[i] = {
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                .DescriptorTable = {
                    .NumDescriptorRanges = static_cast<UINT>(nativeTables[i].size()),
                    .pDescriptorRanges   = nativeTables[i].data(),
                },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            };
        break;
        };
    }
    for (int i = 0; i < samplers.size(); i++) {
        const Sampler& sampler = samplers[i];
        nativeSamplers[i] = {
            .Filter             = getNativeFilter(sampler.filter),
            .AddressU           = getNativeAddressMode(sampler.addressU),
            .AddressV           = getNativeAddressMode(sampler.addressV),
            .AddressW           = getNativeAddressMode(sampler.addressW),
            .MipLODBias         = 0,
            .MaxAnisotropy      = 16,
            .ComparisonFunc     = D3D12_COMPARISON_FUNC_LESS_EQUAL,
            .BorderColor        = getNativeSamplerBorderColor(sampler.borderColor),
            .MinLOD             = 0.f,
            .MaxLOD             = D3D12_FLOAT32_MAX,
            .ShaderRegister     = sampler.shaderRegister,
            .RegisterSpace      = sampler.registerSpace,
            .ShaderVisibility   = D3D12_SHADER_VISIBILITY_ALL
        };
    }
    D3D12_ROOT_SIGNATURE_DESC rootDesc = {
        .NumParameters      = static_cast<UINT>(nativeParams.size()),
        .pParameters        = nativeParams.data(),
        .NumStaticSamplers  = static_cast<UINT>(nativeSamplers.size()),
        .pStaticSamplers    = nativeSamplers.data(),
        .Flags              = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    };
    ASSERT_DX(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
    ASSERT_DX(obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&rootData.obj)));
    return root;
}

Pipeline createPipeline(Root root, const char* shader) {
    Pipeline pipeline = pipelines.alloc();
    PipelineData& pipelineData = pipelines[pipeline];
    Shader s(shader);
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
        .pRootSignature = get(root),
        .CS = s.bytecode(),
        .NodeMask = 0,
        .CachedPSO = { nullptr, 0 },
        .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
    };
    ASSERT_DX(obj->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineData.obj)));
    return pipeline;
}

Pipeline createPipeline(Root root, PsoGraphicsDesc desc) {
    Pipeline pipeline = pipelines.alloc();
    PipelineData& pipelineData = pipelines[pipeline];
    Shader vShader(desc.vs);
    Shader pShader(desc.ps);

    std::vector<D3D12_INPUT_ELEMENT_DESC> nativeInputs(desc.inputElements.size());
    for (int i = 0; i < desc.inputElements.size(); i++) {
        const InputElement& inputElement = desc.inputElements[i];
        nativeInputs[i] = {
            .SemanticName           = inputElement.semanticName,
            .SemanticIndex          = inputElement.semanticIndex,
            .Format                 = getNativeFormat(inputElement.format),
            .InputSlot              = inputElement.inputSlot,
            .AlignedByteOffset      = inputElement.alignedOffset,
            .InputSlotClass         = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            .InstanceDataStepRate   = 0,
        };
    }
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature             = get(root),
        .VS                         = vShader.bytecode(),
        .PS                         = pShader.bytecode(),
        .BlendState                 = {
            .AlphaToCoverageEnable  = FALSE,
            .IndependentBlendEnable = TRUE,
            .RenderTarget           = {
                { .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL },
                { .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL },
                { .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL },
                { .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL },
                { .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL },
                { .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL },
                { .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL },
                { .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL },
            },
        },
        .SampleMask                 = UINT_MAX,
        .RasterizerState            = {
            .FillMode               = getNativeFillMode(desc.fillMode),
            .CullMode               = getNativeCullMode(desc.cullMode),
            .FrontCounterClockwise  = TRUE,
            .DepthBias              = D3D12_DEFAULT_DEPTH_BIAS,
            .DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
            .SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
            .DepthClipEnable        = TRUE,
            .MultisampleEnable      = FALSE,
            .AntialiasedLineEnable  = FALSE,
            .ForcedSampleCount      = 0,
            .ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
        },
        .DepthStencilState          = {
            .DepthEnable            = FALSE,
            .DepthWriteMask         = D3D12_DEPTH_WRITE_MASK_ZERO,
            .DepthFunc              = D3D12_COMPARISON_FUNC_NONE,
            .StencilEnable          = FALSE,
            .StencilReadMask        = D3D12_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask       = D3D12_DEFAULT_STENCIL_WRITE_MASK,
        },
        .InputLayout                = { nativeInputs.data(), static_cast<UINT>(nativeInputs.size()) },
        .IBStripCutValue            = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        .PrimitiveTopologyType      = getNativePrimitiveTopologyType(desc.topologyType),
        .NumRenderTargets           = static_cast<UINT>(desc.renderTargets.size()),
        .RTVFormats                 = {},
        .DSVFormat                  = getNativeFormat(desc.dsvFormat),
        .SampleDesc                 = { desc.samples, 0 },
        .NodeMask                   = 0,
        .CachedPSO                  = { nullptr, 0 },
    };

    if (desc.enableDepth) {
        psoDesc.DepthStencilState.DepthEnable    = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    }
    if (desc.enableStencil) {
        psoDesc.DepthStencilState.StencilEnable      = TRUE;
        psoDesc.DepthStencilState.StencilReadMask    = 0;
        psoDesc.DepthStencilState.StencilWriteMask   = 0;
        psoDesc.DepthStencilState.FrontFace = {
            .StencilFailOp      = D3D12_STENCIL_OP_REPLACE,
            .StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE,
            .StencilPassOp      = D3D12_STENCIL_OP_REPLACE,
            .StencilFunc        = D3D12_COMPARISON_FUNC_GREATER,
        };
        psoDesc.DepthStencilState.BackFace = {
            .StencilFailOp      = D3D12_STENCIL_OP_REPLACE,
            .StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE,
            .StencilPassOp      = D3D12_STENCIL_OP_REPLACE,
            .StencilFunc        = D3D12_COMPARISON_FUNC_GREATER,
        };
    }

    for (int i = 0; i < desc.renderTargets.size(); i++) {
        const RenderTargetDesc& rtDesc = desc.renderTargets[i];
        if (rtDesc.enableBlend) {
            psoDesc.BlendState.RenderTarget[i] = {
                .BlendEnable            = TRUE,
                .LogicOpEnable          = FALSE,
                .SrcBlend               = D3D12_BLEND_SRC_ALPHA,
                .DestBlend              = D3D12_BLEND_INV_SRC_ALPHA,
                .BlendOp                = D3D12_BLEND_OP_ADD,
                .SrcBlendAlpha          = D3D12_BLEND_ONE,
                .DestBlendAlpha         = D3D12_BLEND_INV_SRC_ALPHA,
                .BlendOpAlpha           = D3D12_BLEND_OP_ADD,
                .RenderTargetWriteMask  = D3D12_COLOR_WRITE_ENABLE_ALL,
            };
        }
        psoDesc.RTVFormats[i] = getNativeFormat(rtDesc.format);
    }
    ASSERT_DX(obj->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineData.obj)));
    return pipeline;
}




Swapchain createSwapchain(Queue& queue, void* window, uint32_t w, uint32_t h, uint32_t frameCount) {
    Swapchain sw = swapchains.alloc();
    HWND hwnd = static_cast<HWND>(window);
    SwapchainData& swapchainData = swapchains[sw];
    QueueData& queueData = queues[queue];

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
        .Width       = w,
        .Height      = h,
        .Format      = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc  = { .Count = 1, },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = frameCount,
        .SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags       = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
    };
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFSDesc = {
        .Windowed = TRUE,
    };
    ComPtr<IDXGISwapChain1> swapChain1;
    ASSERT_DX(factory->CreateSwapChainForHwnd(queueData.obj.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
    ASSERT_DX(swapChain1.As(&swapchainData.obj));
    ASSERT_DX(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

    swapchainData.frameIdx = swapchainData.obj->GetCurrentBackBufferIndex();
    swapchainData.targets.resize(frameCount);

    for (int i = 0; i < frameCount; i++) {
        SwapTarget& target = swapchainData.targets[i];
        target.texture = textures.alloc();
        TextureData& textureData = textures[target.texture];
        std::string name = "swapchain_" + std::to_string(i);

        ASSERT_DX(swapchainData.obj->GetBuffer(i, IID_PPV_ARGS(&textureData.obj)));
        // D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {
        //     .Format         = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        //     .ViewDimension  = D3D12_RTV_DIMENSION_TEXTURE2D,
        //     .Texture2D = {
        //         .MipSlice   = 0,
        //         .PlaneSlice = 0,
        //     }
        // };
        setResourceName(textureData.obj.Get(), name.c_str());
        target.renderView = createRenderView(target.texture);
    }

    return sw;
}

void present(Swapchain sw, bool vsync) {
    SwapchainData& swapchainData = swapchains[sw];
    if (vsync) {
        ASSERT_DX(swapchainData.obj->Present(1, 0));
    } else {
        ASSERT_DX(swapchainData.obj->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    }
}

SwapTarget next(Swapchain sw) {
    SwapchainData& swapchainData = swapchains[sw];
    swapchainData.frameIdx = swapchainData.obj->GetCurrentBackBufferIndex();
    return swapchainData.targets[swapchainData.frameIdx];
}


























Buffer createBuffer(const char* name, BufferUsage usage, uint32_t size) {
    Buffer buffer = buffers.alloc(name);
    BufferData& bufferData = buffers[buffer];

    bufferData.size  = size;
    switch(usage) {
    case BufferUpload:
        bufferData.heap  = D3D12_HEAP_TYPE_UPLOAD;
        bufferData.state = D3D12_RESOURCE_STATE_GENERIC_READ;
        bufferData.flags = D3D12_RESOURCE_FLAG_NONE;
    break;
    case BufferDefault:
        bufferData.heap  = D3D12_HEAP_TYPE_DEFAULT;
        bufferData.state = D3D12_RESOURCE_STATE_COMMON;
        bufferData.flags = D3D12_RESOURCE_FLAG_NONE;
    case BufferWrite:
        bufferData.heap  = D3D12_HEAP_TYPE_DEFAULT;
        bufferData.state = D3D12_RESOURCE_STATE_COMMON;
        bufferData.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    break;
    case BufferRead:
        bufferData.heap  = D3D12_HEAP_TYPE_READBACK;
        bufferData.state = D3D12_RESOURCE_STATE_COMMON;
        bufferData.flags = D3D12_RESOURCE_FLAG_NONE;
    break;
    }

    D3D12_HEAP_PROPERTIES props = {
        .Type                   = bufferData.heap,
        .CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask       = 1,
        .VisibleNodeMask        = 1,
    };
    D3D12_RESOURCE_DESC resDesc = {
        .Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment          = 0,
        .Width              = size,
        .Height             = 1,
        .DepthOrArraySize   = 1,
        .MipLevels          = 1,
        .Format             = DXGI_FORMAT_UNKNOWN,
        .SampleDesc         = { 1, 0 },
        .Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags              = bufferData.flags,
    };
    ASSERT_DX(obj->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &resDesc, bufferData.state, nullptr, IID_PPV_ARGS(&bufferData.obj)));

    if (usage == BufferUpload) {
        D3D12_RANGE readRange = { 0, 0 }; // We do not intend to read from this resource on the CPU.
        ASSERT_DX(bufferData.obj->Map(0, &readRange, reinterpret_cast<void**>(&bufferData.data)));
    }
    setResourceName(bufferData.obj.Get(), name);
    return buffer;
}

void write(Buffer buffer, uint32_t offset, uint32_t size, const uint8_t* data) {
    BufferData& bufferData = buffers[buffer];
    ASSERT(bufferData.heap == D3D12_HEAP_TYPE_UPLOAD);
    ASSERT(bufferData.size >= offset + size);
    memcpy(bufferData.data + offset, data, size);
}

void read(Buffer buffer, const std::function<void(uint8_t*)>& f) {
    BufferData& bufferData = buffers[buffer];
    ASSERT(bufferData.heap == D3D12_HEAP_TYPE_READBACK);
    uint8_t* data = nullptr;
    bufferData.obj->Map(0, nullptr, reinterpret_cast<void**>(&data));
    f(data);
    bufferData.obj->Unmap(0, nullptr);
}



























Texture createTexture(const char* name, const uint8_t* data, uint32_t size, std::vector<ResourceData>& subresources) {
    Texture texture = textures.alloc(name);
    TextureData& textureData = textures[texture];
    std::vector<D3D12_SUBRESOURCE_DATA> nativeSubresources;
    ASSERT_DX(LoadDDSTextureFromMemory(obj.Get(), data, size, textureData.obj.GetAddressOf(), nativeSubresources));

    subresources.resize(nativeSubresources.size());
    for (int i = 0; i < subresources.size(); i++) {
        const D3D12_SUBRESOURCE_DATA& sub = nativeSubresources[i];
        subresources[i] = ResourceData { sub.pData, sub.RowPitch, sub.SlicePitch };
    }
    return texture;
}

Texture createTexture1(TextureUsage usage, Format format, uvec3 dims, uint16_t mips, ClearValue value) {
    Texture texture = textures.alloc();
    TextureData& textureData = textures[texture];
    return texture;
}



Texture createTexture2(const char* name, TextureUsage usage, Format format, uvec3 dims, uint16_t mips, uint8_t samples, ClearValue value) {
    Texture texture = textures.alloc(name);
    TextureData& textureData = textures[texture];

    D3D12_RESOURCE_STATES state = {};
    D3D12_RESOURCE_FLAGS  flags = {};
    DXGI_FORMAT nativeFormat = getNativeFormat(format);
    D3D12_CLEAR_VALUE nativeClearValue = { .Format = nativeFormat, };
    D3D12_CLEAR_VALUE* nativeClearValuePtr = nullptr;

    bool isDepth  = hasFlag(usage, TextureUsage::DepthStencil);
    bool isRender = hasFlag(usage, TextureUsage::RenderTarget);
    if (isDepth) {
        state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        nativeClearValue = {
            .Format = nativeFormat,
            .DepthStencil = { .Depth = value.depth, .Stencil = value.stencil },
        };
        nativeClearValuePtr = &nativeClearValue;
    }
    if (isRender) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        nativeClearValue = {
            .Format = nativeFormat,
            .Color = { value.color.x, value.color.y, value.color.z, value.color.w },
        };
        nativeClearValuePtr = &nativeClearValue;
    }
    if (isDepth && isRender) {

    }
    // TODO: should use DXGI_FORMAT_R24G8_TYPELESS in texture creation
    // desc.Format              = DXGI_FORMAT_R24G8_TYPELESS;
    // clearValue.Format        = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // dsvDesc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // srvDesc.Format           = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

    textureData.state = state;
    D3D12_HEAP_PROPERTIES props = {
        .Type                   = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask       = 1,
        .VisibleNodeMask        = 1,
    };
    D3D12_RESOURCE_DESC resDesc = {
        .Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment          = 0,
        .Width              = dims.x,
        .Height             = dims.y,
        .DepthOrArraySize   = static_cast<uint16_t>(dims.z),
        .MipLevels          = mips,
        .Format             = nativeFormat,
        .SampleDesc         = { samples, 0 },
        .Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags              = flags,
    };
    ASSERT_DX(obj->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &resDesc, state, nativeClearValuePtr, IID_PPV_ARGS(&textureData.obj)));
    setResourceName(textureData.obj.Get(), name);
    return texture;
}

Texture createTexture3(TextureUsage usage, Format format, uvec3 dims, uint16_t mips, ClearValue value) {
    Texture texture = textures.alloc();
    TextureData& textureData = textures[texture];
    return texture;
}

//         device->CreateConstantBufferView(&desc, descriptor);
//         device->CreateUnorderedAccessView(resource, nullptr, &desc, descriptor);

TextureView createTextureView(Texture texture) {
    ASSERT(colorViewCount < maxColorViews);
    colorViewCount++;
    TextureData& textureData = textures[texture];
    D3D12_RESOURCE_DESC texDesc = textureData.obj->GetDesc();

    // depth format adjustement
    DXGI_FORMAT format = texDesc.Format;
    switch(texDesc.Format) {
        case DXGI_FORMAT_D16_UNORM           : format = DXGI_FORMAT_R16_UNORM;                  break;
        case DXGI_FORMAT_D24_UNORM_S8_UINT   : format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;      break;
        case DXGI_FORMAT_D32_FLOAT           : format = DXGI_FORMAT_R32_FLOAT;                  break;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;   break;
        default                              :                                                  break;
    }

    // Currently the sRGB format seems to have no effect on sampling in shaders.
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {
        .Format = format,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = texDesc.MipLevels,
            .PlaneSlice = 0,
            .ResourceMinLODClamp = 0.0f,
        }
    };
    auto [idx, descriptor] = mainHeap.alloc();
    obj->CreateShaderResourceView(get(texture), &desc, descriptor);
    TextureView textureView = textureViews.alloc();
    TextureViewData& textureViewData = textureViews[textureView];
    textureViewData.aspect = TextureViewColor;
    textureViewData.idx = idx;
    return textureView;
}

TextureView createDepthView(Texture texture) {
    ASSERT(depthViewCount < maxDepthViews);
    depthViewCount++;
    TextureData& textureData = textures[texture];
    D3D12_RESOURCE_DESC texDesc = textureData.obj->GetDesc();
    D3D12_DSV_DIMENSION dsvDim;
    ASSERT(texDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    if (texDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) {
        dsvDim = texDesc.DepthOrArraySize > 1 ? D3D12_DSV_DIMENSION_TEXTURE1DARRAY : D3D12_DSV_DIMENSION_TEXTURE1D;
    } else if (texDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        dsvDim = texDesc.DepthOrArraySize > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DARRAY : D3D12_DSV_DIMENSION_TEXTURE2D;
    }
    DXGI_FORMAT dsvFormat = texDesc.Format;
    if (texDesc.Format == DXGI_FORMAT_R32G8X24_TYPELESS) {
        dsvFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    } else if (texDesc.Format == DXGI_FORMAT_R24G8_TYPELESS) {
        dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    }
    ASSERT( dsvFormat == DXGI_FORMAT_D16_UNORM             ||
            dsvFormat == DXGI_FORMAT_D24_UNORM_S8_UINT     ||
            dsvFormat == DXGI_FORMAT_D32_FLOAT             ||
            dsvFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT
    );
    D3D12_DEPTH_STENCIL_VIEW_DESC desc = {
        .Format         = texDesc.Format,
        .ViewDimension  = dsvDim,
        .Flags          = D3D12_DSV_FLAG_NONE,
        .Texture2D      = { .MipSlice = 0 },
    };

    auto [idx, descriptor] = dsvHeap.alloc();
    obj->CreateDepthStencilView(get(texture), &desc, descriptor);
    TextureView textureView = textureViews.alloc();
    TextureViewData& textureViewData = textureViews[textureView];
    textureViewData.aspect = TextureViewDepth;
    textureViewData.idx = idx;
    return textureView;
}

TextureView createRenderView(Texture texture) {
    ASSERT(renderViewCount < maxRenderViews);
    renderViewCount++;
    auto [ idx, descriptor ] = rtvHeap.alloc();
    obj->CreateRenderTargetView(get(texture), nullptr, descriptor);
    TextureView textureView = textureViews.alloc();
    TextureViewData& textureViewData = textureViews[textureView];
    textureViewData.aspect = TextureViewRender;
    textureViewData.idx = idx;
    return textureView;
}

int getTextureViewBind(TextureView textureView) {
    return textureViews[textureView].idx;
}

void destroy(Buffer buffer) {
    buffers.free(buffer);
}

void destroy(Texture texture) {
    textures.free(texture);
}

void destroy(TextureView textureView) {
    TextureViewData& textureViewData = textureViews[textureView];
    switch(textureViewData.aspect) {
        case TextureViewColor:  mainHeap.free(textureViewData.idx);  colorViewCount--; break;
        case TextureViewDepth:   dsvHeap.free(textureViewData.idx);  depthViewCount--; break;
        case TextureViewRender:  rtvHeap.free(textureViewData.idx); renderViewCount--; break;
    }
    textureViews.free(textureView);
}




















////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS /////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
private:

    ID3D12GraphicsCommandList* get(Command command) {
        return commands[command].cmdList.Get();
    }

    ID3D12RootSignature* get(Root root) {
        return roots[root].obj.Get();
    }

    ID3D12PipelineState* get(Pipeline pipeline) {
        return pipelines[pipeline].obj.Get();
    }

    ID3D12Resource* get(Buffer buffer) {
        return buffers[buffer].obj.Get();
    }

    ID3D12Resource* get(Texture texture) {
        return textures[texture].obj.Get();
    }

    D3D12_GPU_VIRTUAL_ADDRESS addr(Buffer buffer) {
        BufferData& bufferData = buffers[buffer];
        return buffers[buffer].obj->GetGPUVirtualAddress();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE textureViewCpu(TextureView textureView) {
        TextureViewData& textureViewData = textureViews[textureView];
        ASSERT(textureViewData.aspect == TextureViewColor);
        return mainHeap.cpu(textureViewData.idx);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE depthViewCpu(TextureView textureView) {
        TextureViewData& textureViewData = textureViews[textureView];
        ASSERT(textureViewData.aspect == TextureViewDepth);
        return dsvHeap.cpu(textureViewData.idx);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE renderViewCpu(TextureView textureView) {
        TextureViewData& textureViewData = textureViews[textureView];
        ASSERT(textureViewData.aspect == TextureViewRender);
        return rtvHeap.cpu(textureViewData.idx);
    }


    Microsoft::WRL::ComPtr<IDXGIFactory4>       factory;
    std::vector<Adapter>                        adapters;
    Adapter*                                    selectedAdapter = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Device>        obj;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue1>    iq;
    Heap                                        mainHeap;
    Heap                                        rtvHeap;
    Heap                                        dsvHeap;
    Vec<Queue,       QueueData>                 queues;
    Vec<Command,     CommandData>               commands;
    Vec<Root,        RootData>                  roots;
    Vec<Pipeline,    PipelineData>              pipelines;
    Vec<Swapchain,   SwapchainData>             swapchains;
    Vec<Buffer,      BufferData>                buffers;
    Vec<Texture,     TextureData>               textures;
    Vec<TextureView, TextureViewData>           textureViews;
    uint32_t                                    colorViewCount;
    uint32_t                                    depthViewCount;
    uint32_t                                    renderViewCount;
    uint32_t                                    maxColorViews;
    uint32_t                                    maxDepthViews;
    uint32_t                                    maxRenderViews;
    uint32_t                                    maxTextureViews;


};
































// // Type unsafe Constant Buffer class, use derived ConstantBuffer.
// class ConstantBufferBase {
// public:
//     ~ConstantBufferBase();
//     bool init(ID3D12Device* gpu, uint32_t elementCount, uint32_t elementSize);
//     ID3D12Resource* getRes();
//     void set(int idx, void* element);
//     D3D12_GPU_VIRTUAL_ADDRESS getAddress(int idx);
//     D3D12_CONSTANT_BUFFER_VIEW_DESC getView(int idx);
    
// private:
//     ID3D12Device* gpu;
//     Microsoft::WRL::ComPtr<ID3D12Resource> obj;
//     uint8_t* data;
//     uint32_t elementCount;
//     uint32_t elementSize;
//     uint32_t elementSizePadded;
    
// };



// template<typename T>
// class ConstantBuffer : public ConstantBufferBase {
// public:

//     bool init(ID3D12Device* gpu, uint32_t elementCount) {
//         return ConstantBufferBase::init(gpu, elementCount, sizeof(T));
//     }

//     void set(int idx, T& element) {
//         ConstantBufferBase::set(idx, &element);
//     }
// };





// ConstantBufferBase::~ConstantBufferBase() {
//     if (obj != nullptr) {
//         obj->Unmap(0, nullptr);
//     }
//     data = nullptr;
// }

// bool ConstantBufferBase::init(ID3D12Device* gpu, uint32_t elementCount, uint32_t elementSize) {
//     this->gpu = gpu;
//     this->elementCount = elementCount;
//     this->elementSize = elementSize;
//     this->elementSizePadded = align256(elementSize);
//     CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
//     auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(elementSizePadded * elementCount);
//     HRESULT res = gpu->CreateCommittedResource(
//         &heapProps,
//         D3D12_HEAP_FLAG_NONE,
//         &resDesc,
//         D3D12_RESOURCE_STATE_GENERIC_READ,
//         nullptr,
//         IID_PPV_ARGS(&obj)
//     );
//     obj->Map(0, nullptr, reinterpret_cast<void**>(&data));
//     return true;
// }

// ID3D12Resource* ConstantBufferBase::getRes() {
//     return obj.Get();
// }

// void ConstantBufferBase::set(int idx, void* element) {
//     memcpy(data + elementSizePadded * idx, element, elementSize);
// }

// D3D12_GPU_VIRTUAL_ADDRESS ConstantBufferBase::getAddress(int idx) {
//     return obj.Get()->GetGPUVirtualAddress() + idx * elementSizePadded;
// }

// D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBufferBase::getView(int idx) {
//     return {
//         .BufferLocation = obj->GetGPUVirtualAddress() + idx * elementSizePadded,
//         .SizeInBytes = elementSizePadded,
//     };
// }




void beginEvent(const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    PIXBeginEvent(PIX_COLOR_DEFAULT, "%s", buffer);
}

void endEvent() {
    PIXEndEvent();
}





std::unique_ptr<IGpu> createGpuD3D(const GpuDesc& desc) {
    return std::make_unique<GpuD3D>(desc);
}

}