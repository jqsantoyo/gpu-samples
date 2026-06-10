#include "gpu.h"
#include <utils/utils.h>
#include <ddsTextureLoader/DDSTextureLoader12.h>
#include <d3dx12.h>
using namespace DirectX;
using namespace Microsoft::WRL;

#define ASSERT_DX(x) { assert(SUCCEEDED(x)); }
#define ASSERT(x)    { assert(x); }

namespace gpu::gpu {



D3D12_BLEND_DESC noBlend() {
    return {
        .AlphaToCoverageEnable  = FALSE,
        .IndependentBlendEnable = FALSE,
        .RenderTarget           = {
            noBlendRt(), noBlendRt(), noBlendRt(), noBlendRt(),
            noBlendRt(), noBlendRt(), noBlendRt(), noBlendRt(),
        },
    };
}


D3D12_BLEND_DESC defaultBlend() {
    return {
        .AlphaToCoverageEnable  = FALSE,
        .IndependentBlendEnable = FALSE,
        .RenderTarget           = {
            defaultBlendRt(), defaultBlendRt(), defaultBlendRt(), defaultBlendRt(),
            defaultBlendRt(), defaultBlendRt(), defaultBlendRt(), defaultBlendRt(),
        },
    };
}

D3D12_RENDER_TARGET_BLEND_DESC noBlendRt() {
    return {
        .BlendEnable            = FALSE,
        .LogicOpEnable          = FALSE,
        .SrcBlend               = D3D12_BLEND_ONE,
        .DestBlend              = D3D12_BLEND_ZERO,
        .BlendOp                = D3D12_BLEND_OP_ADD,
        .SrcBlendAlpha          = D3D12_BLEND_ONE,
        .DestBlendAlpha         = D3D12_BLEND_ZERO,
        .BlendOpAlpha           = D3D12_BLEND_OP_ADD,
        .LogicOp                = D3D12_LOGIC_OP_NOOP,
        .RenderTargetWriteMask  = D3D12_COLOR_WRITE_ENABLE_ALL,
    };
}
D3D12_RENDER_TARGET_BLEND_DESC defaultBlendRt() {
    return {
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

D3D12_RASTERIZER_DESC defaultWire() {
    return {
        .FillMode               = D3D12_FILL_MODE_WIREFRAME,
        .CullMode               = D3D12_CULL_MODE_NONE,
        .FrontCounterClockwise  = FALSE,
        .DepthBias              = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable        = TRUE,
        .MultisampleEnable      = FALSE,
        .AntialiasedLineEnable  = FALSE,
        .ForcedSampleCount      = 0,
        .ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };
}

D3D12_RASTERIZER_DESC defaultFill() {
    return {
        .FillMode               = D3D12_FILL_MODE_SOLID,
        .CullMode               = D3D12_CULL_MODE_NONE,
        .FrontCounterClockwise  = FALSE,
        .DepthBias              = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable        = TRUE,
        .MultisampleEnable      = FALSE,
        .AntialiasedLineEnable  = FALSE,
        .ForcedSampleCount      = 0,
        .ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };
}

D3D12_DEPTH_STENCIL_DESC noDepthStencil() {
    return {
        .DepthEnable        = FALSE,
        .DepthWriteMask     = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc          = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable      = FALSE,
        // .StencilReadMask    = 0,
        // .StencilWriteMask   = 0,
        // .FrontFace = {
        //     .StencilFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilPassOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilFunc = D3D12_COMPARISON_FUNC_GREATER,
        // },
        // .BackFace = {
        //     .StencilFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilPassOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilFunc = D3D12_COMPARISON_FUNC_GREATER,
        // },
    };
}

D3D12_DEPTH_STENCIL_DESC defaultDepth() {
    return {
        .DepthEnable        = TRUE,
        .DepthWriteMask     = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc          = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable      = FALSE,
    };
}

D3D12_STATIC_SAMPLER_DESC anisotropicSampler() {
    return CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC);
}

D3D12_STATIC_SAMPLER_DESC shadowSampler() {
    return {
        .Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 16,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
        .MinLOD = 0.f,
        .MaxLOD = D3D12_FLOAT32_MAX,
        .ShaderRegister = 1,
        .RegisterSpace = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
    };
}


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




D3D12_ROOT_PARAMETER rootCbv(
    UINT shaderRegister,
    UINT registerSpace,
    D3D12_SHADER_VISIBILITY visibility
) {
    return {
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
        .Descriptor = {
            .ShaderRegister = shaderRegister,
            .RegisterSpace = registerSpace,
        },
        .ShaderVisibility = visibility,
    };
}

D3D12_ROOT_PARAMETER rootSrv(
    UINT shaderRegister,
    UINT registerSpace,
    D3D12_SHADER_VISIBILITY visibility
) {
    return {
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
        .Descriptor = {
            .ShaderRegister = shaderRegister,
            .RegisterSpace = registerSpace,
        },
        .ShaderVisibility = visibility,
    };
}

D3D12_ROOT_PARAMETER rootUav(
    UINT shaderRegister,
    UINT registerSpace,
    D3D12_SHADER_VISIBILITY visibility
) {
    return {
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
        .Descriptor = {
            .ShaderRegister = shaderRegister,
            .RegisterSpace = registerSpace,
        },
        .ShaderVisibility = visibility,
    };
}

D3D12_ROOT_PARAMETER rootTable(
    std::initializer_list<D3D12_DESCRIPTOR_RANGE> ranges,
    D3D12_SHADER_VISIBILITY visibility
) {
    return {
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
        .DescriptorTable = {
            .NumDescriptorRanges = static_cast<UINT>(ranges.size()),
            .pDescriptorRanges = ranges.begin(),
        },
        .ShaderVisibility = visibility,
    };
}

D3D12_DESCRIPTOR_RANGE rangeSrv(
    UINT numDescriptors,
    UINT baseShaderRegister,
    UINT registerSpace,
    UINT offset
) {
    return {
        .RangeType                          = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        .NumDescriptors                     = numDescriptors,
        .BaseShaderRegister                 = baseShaderRegister,
        .RegisterSpace                      = registerSpace,
        .OffsetInDescriptorsFromTableStart  = offset,
    };
}

D3D12_DESCRIPTOR_RANGE rangeUav(
    UINT numDescriptors,
    UINT baseShaderRegister,
    UINT registerSpace,
    UINT offset
) {
    return {
        .RangeType                          = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
        .NumDescriptors                     = numDescriptors,
        .BaseShaderRegister                 = baseShaderRegister,
        .RegisterSpace                      = registerSpace,
        .OffsetInDescriptorsFromTableStart  = offset,
    };
}

D3D12_DESCRIPTOR_RANGE rangeCbv(
    UINT numDescriptors,
    UINT baseShaderRegister,
    UINT registerSpace,
    UINT offset
) {
    return {
        .RangeType                          = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
        .NumDescriptors                     = numDescriptors,
        .BaseShaderRegister                 = baseShaderRegister,
        .RegisterSpace                      = registerSpace,
        .OffsetInDescriptorsFromTableStart  = offset,
    };
}








bool Heap::init(ID3D12Device* gpu, const char* name, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, std::initializer_list<uint32_t> pageCapacities) {
    this->name = name;
    UINT totalCount = 0;
    const uint32_t* capacity = pageCapacities.begin();
    pages.resize(pageCapacities.size());
    for (int i = 0; i < pages.size(); i++) {
        pages[i] = { .offset = totalCount, .capacity = *capacity, .head = -1 };
        totalCount += *capacity;
        capacity++;
    }
    size = gpu->GetDescriptorHandleIncrementSize(type);
    D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type           = type,
        .NumDescriptors = totalCount,
        .Flags          = flags,
        .NodeMask       = 0,
    };
    ASSERT_DX(gpu->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&obj)));
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE Heap::cpu(UINT idx) {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(obj->GetCPUDescriptorHandleForHeapStart(), idx, size);
}


D3D12_GPU_DESCRIPTOR_HANDLE Heap::gpu(UINT idx) {
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(obj->GetGPUDescriptorHandleForHeapStart(), idx, size);
}

ID3D12DescriptorHeap* Heap::get() {
    return obj.Get();
}

int Heap::next(int pageIdx) {
    Page& page = pages[pageIdx];
    page.head++;
    ASSERT(page.head < page.capacity);
    // printf("Reached heap limit. Heap: \"%s\", page: %d, capacity: %d\n", name, pageIdx, page.capacity);
    int idx = page.offset + page.head;
    return idx;
}

void Heap::reset(int pageIdx) {
    pages[pageIdx].head = -1;
}



Cbv Gpu::nextCbv(int page) {
    Cbv cbv;
    return cbv;
}

Srv Gpu::nextSrv(int page, Texture texture, D3D12_SHADER_RESOURCE_VIEW_DESC& desc) {
    Srv srv;
    srv.idx = mainHeap.next(page);
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor = mainHeap.cpu(srv.idx);
    obj->CreateShaderResourceView(get(texture), &desc, descriptor);
    return srv;
}

Uav Gpu::nextUav(int page) {
    Uav uav;
    return uav;
}

Dsv Gpu::nextDsv(int page, Texture texture, D3D12_DEPTH_STENCIL_VIEW_DESC& desc) {
    Dsv dsv;
    dsv.idx = dsvHeap.next(page);
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor = dsvHeap.cpu(dsv.idx);
    printf("DSV ptr = 0x%llx\n", descriptor.ptr);
    obj->CreateDepthStencilView(get(texture), &desc, descriptor);
    return dsv;
}

Rtv Gpu::nextRtv(int page, Texture texture) {
    Rtv rtv;
    rtv.idx = rtvHeap.next(page);
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor = rtvHeap.cpu(rtv.idx);
    obj->CreateRenderTargetView(get(texture), nullptr, descriptor);
    return rtv;
}


void Gpu::resetMain(int page) {
    mainHeap.reset(page);
}

void Gpu::resetDsv(int page) {
    dsvHeap.reset(page);
}

void Gpu::resetRtv(int page) {
    rtvHeap.reset(page);
}



void deviceMessage( 
    D3D12_MESSAGE_CATEGORY Category,
    D3D12_MESSAGE_SEVERITY Severity,
    D3D12_MESSAGE_ID ID,
    LPCSTR pDescription,
    void *pContext
) {
    printf("Gpu message\n");
}



void Gpu::init(UINT rtvCount, UINT dsvCount, UINT cbvCount) {

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


    if (rtvCount > 0) {
        rtvHeap.init(obj.Get(), "RTV", D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, {rtvCount});
    }
    if (dsvCount > 0) {
        dsvHeap.init(obj.Get(), "DSV", D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, {dsvCount});
    }
    if (cbvCount > 0) {
        mainHeap.init(obj.Get(), "CBV-SRV-UAV", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, {cbvCount});
    }
}

void Gpu::print() {
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


void Gpu::printErrors() {
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


Queue Gpu::createQueue(D3D12_COMMAND_LIST_TYPE type) {
    Queue queue = queues.add();
    QueueData& queueData = queues[queue];
    D3D12_COMMAND_QUEUE_DESC desc = {
        .Type       = type,
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

void Gpu::execute(Queue queue, Command& command) {
    QueueData& queueData = queues[queue];
    CommandData& commandData = commands[command];
    ID3D12CommandList* cmdLists[] = { commandData.cmdList.Get() };
    queueData.obj->ExecuteCommandLists(1, cmdLists);
}

void Gpu::signal(Queue queue, Command& command) {
    QueueData& queueData = queues[queue];
    CommandData& commandData = commands[command];
    signal(queue, commandData.fenceValue);
}

void Gpu::wait(Queue queue, Command& command) {
    QueueData& queueData = queues[queue];
    CommandData& commandData = commands[command];
    wait(queue, commandData.fenceValue);
}

void Gpu::signal(Queue queue, uint64_t& value) {
    QueueData& queueData = queues[queue];
    queueData.nextFenceValue++;
    value = queueData.nextFenceValue;
    ASSERT_DX(queueData.obj->Signal(queueData.fence.Get(), queueData.nextFenceValue));

}

void Gpu::wait(Queue queue, uint64_t value) {
    QueueData& queueData = queues[queue];
    if (queueData.fence->GetCompletedValue() < value) {
        ASSERT_DX(queueData.fence->SetEventOnCompletion(value, queueData.fenceEvent));
        WaitForSingleObject(queueData.fenceEvent, INFINITE);
    }
}


void Gpu::wait(Queue queue) {
    UINT64 value;
    signal(queue, value);
    wait(queue, value);
}
































Command Gpu::createCommand(D3D12_COMMAND_LIST_TYPE type, uint32_t maxMemory) {
    Command command = commands.add();
    CommandData& commandData = commands[command];
    ASSERT_DX(obj->CreateCommandAllocator(type, IID_PPV_ARGS(&commandData.cmdAllocator)));
    ASSERT_DX(obj->CreateCommandList(0, type, commandData.cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&commandData.cmdList)));
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

void Gpu::begin(Command command, Pipeline pipeline) {
    CommandData& commandData = commands[command];
    ID3D12PipelineState* initialState = pipeline.idx != -1 ? pipelines[pipeline].obj.Get() : nullptr;
    ASSERT_DX(commandData.cmdAllocator->Reset());
    ASSERT_DX(commandData.cmdList->Reset(commandData.cmdAllocator.Get(), initialState));
    commandData.allocatedMemory = 0;
}

void Gpu::viewport(Command command, D3D12_VIEWPORT& viewport) {
    CommandData& commandData = commands[command];
    commandData.cmdList->RSSetViewports(1, &viewport);
}

void Gpu::scissor(Command command, D3D12_RECT& rect) {
    CommandData& commandData = commands[command];
    commandData.cmdList->RSSetScissorRects(1, &rect);
}

void Gpu::targets(Command command, Rtv rtv, Dsv dsv) {
    CommandData& commandData = commands[command];
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDes = rtvHeap.cpu(rtv.idx);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDes = dsvHeap.cpu(dsv.idx);
    commandData.cmdList->OMSetRenderTargets(1, &rtvDes, true, &dsvDes);
}

void Gpu::target(Command command, Dsv dsv) {
    CommandData& commandData = commands[command];
    D3D12_CPU_DESCRIPTOR_HANDLE des = dsvHeap.cpu(dsv.idx);
    commandData.cmdList->OMSetRenderTargets(0, nullptr, false, &des);
}


void Gpu::clear(Command command, Rtv rtv, vec4 color) {
    CommandData& commandData = commands[command];
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDes = rtvHeap.cpu(rtv.idx);
    commandData.cmdList->ClearRenderTargetView(rtvDes, &color.x, 0, nullptr);
}

void Gpu::clear(Command command, Dsv dsv, float depth, uint8_t stencil) {
    CommandData& commandData = commands[command];
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDes = dsvHeap.cpu(dsv.idx);
    commandData.cmdList->ClearDepthStencilView(dsvDes, D3D12_CLEAR_FLAG_DEPTH, depth, stencil, 0, nullptr);
}


void Gpu::computeRoot(Command command, Root& root) {
    CommandData& commandData = commands[command];
    RootData& rootData = roots[root];
    commandData.cmdList->SetComputeRootSignature(rootData.obj.Get());
}

void Gpu::computeSrv(Command command, int idx, Buffer& buffer) {
    CommandData& commandData = commands[command];
    BufferData& bufferData = buffers[buffer];
    commandData.cmdList->SetComputeRootShaderResourceView(idx, bufferData.obj->GetGPUVirtualAddress());
}

void Gpu::computeUav(Command command, int idx, Buffer& buffer) {
    CommandData& commandData = commands[command];
    BufferData& bufferData = buffers[buffer];
    commandData.cmdList->SetComputeRootUnorderedAccessView(idx, bufferData.obj->GetGPUVirtualAddress());
}

void Gpu::graphicsRoot(Command command, Root& root) {
    CommandData& commandData = commands[command];
    RootData& rootData = roots[root];
    commandData.cmdList->SetGraphicsRootSignature(rootData.obj.Get());
}

void Gpu::graphicsCbv(Command command, int idx, D3D12_GPU_VIRTUAL_ADDRESS addr) {
    CommandData& commandData = commands[command];
    commandData.cmdList->SetGraphicsRootConstantBufferView(idx, addr);
}

void Gpu::graphicsCbv(Command command, int idx, void* data, int dataSize) {
    CommandData& commandData = commands[command];
    memcpy(commandData.data + commandData.allocatedMemory, data, dataSize);
    D3D12_GPU_VIRTUAL_ADDRESS addr = commandData.constantBuffer->GetGPUVirtualAddress() + commandData.allocatedMemory;
    commandData.cmdList->SetGraphicsRootConstantBufferView(idx, addr);
    commandData.allocatedMemory += dataSize;
}

void Gpu::graphicsTable(Command command, int idx, D3D12_GPU_DESCRIPTOR_HANDLE descriptor) {
    CommandData& commandData = commands[command];
    commandData.cmdList->SetGraphicsRootDescriptorTable(idx, descriptor);
}

void Gpu::dispatch(Command command, uint32_t groupsX, uint32_t  groupsY, uint32_t groupsZ) {
    CommandData& commandData = commands[command];
    commandData.cmdList->Dispatch(groupsX, groupsY, groupsZ);
}

void Gpu::copy(Command command, Buffer& dst, Buffer& src) {
    CommandData& commandData = commands[command];
    ID3D12Resource* dstRes = get(dst);
    ID3D12Resource* srcRes = get(src);
    auto barr0 = CD3DX12_RESOURCE_BARRIER::Transition(srcRes, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto barr1 = CD3DX12_RESOURCE_BARRIER::Transition(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
    commandData.cmdList->ResourceBarrier(1, &barr0);
    commandData.cmdList->CopyResource(dstRes, srcRes);
    commandData.cmdList->ResourceBarrier(1, &barr1);
}

void Gpu::subresources(Command command, Texture texture, Buffer uploadBuffer, int offset, int subresourceOffset, const std::vector<D3D12_SUBRESOURCE_DATA>& subresources) {
    UpdateSubresources(get(command), get(texture), get(uploadBuffer), offset, subresourceOffset, subresources.size(), subresources.data());
}

void Gpu::barrier(Command command, Buffer buffer, D3D12_RESOURCE_STATES state) {
    CommandData& commandData = commands[command];
    BufferData& bufferData = buffers[buffer];
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(get(buffer), bufferData.state, state);
    bufferData.state = state;
    commandData.cmdList->ResourceBarrier(1, &barrier);
}

void Gpu::barrier(Command command, Texture texture, D3D12_RESOURCE_STATES state) {
    CommandData& commandData = commands[command];
    TextureData& textureData = textures[texture];
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(get(texture), textureData.state, state);
    textureData.state = state;
    commandData.cmdList->ResourceBarrier(1, &barrier);
}

void Gpu::heaps(Command command, ID3D12DescriptorHeap* mainHeap, ID3D12DescriptorHeap* samplerHeap) {
    CommandData& commandData = commands[command];
    ID3D12DescriptorHeap* heaps[] = { mainHeap, samplerHeap };
    int heapCount = samplerHeap == nullptr ? 1 : 2;
    commandData.cmdList->SetDescriptorHeaps(heapCount, heaps);
}

void Gpu::pipeline(Command command, Pipeline pipeline) {
    CommandData& commandData = commands[command];
    PipelineData& pipelineData = pipelines[pipeline];
    commandData.cmdList->SetPipelineState(pipelineData.obj.Get());
}

void Gpu::topology(Command command, D3D12_PRIMITIVE_TOPOLOGY topology) {
    CommandData& commandData = commands[command];
    commandData.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Gpu::vertexBuffer(Command command, uint32_t slot, D3D12_VERTEX_BUFFER_VIEW view) {
    CommandData& commandData = commands[command];
    commandData.cmdList->IASetVertexBuffers(slot, 1, &view);
}
void Gpu::indexBuffer(Command command, D3D12_INDEX_BUFFER_VIEW view) {
    CommandData& commandData = commands[command];
    commandData.cmdList->IASetIndexBuffer(&view);
}

void Gpu::drawIndexed(Command command, UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation) {
    CommandData& commandData = commands[command];
    commandData.cmdList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void Gpu::draw(Command command, UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation) {
    CommandData& commandData = commands[command];
    commandData.cmdList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void Gpu::end(Command command) {
    CommandData& commandData = commands[command];
    ASSERT_DX(commandData.cmdList->Close());
}
















Root Gpu::createRoot(std::initializer_list<D3D12_ROOT_PARAMETER> params, std::initializer_list<D3D12_STATIC_SAMPLER_DESC> samplers) {
    Root root = roots.add();
    RootData& rootData = roots[root];
    ComPtr<ID3DBlob>            sig;
    ComPtr<ID3DBlob>            error;
    CD3DX12_ROOT_SIGNATURE_DESC desc;
    desc.Init(params.size(), params.begin(), samplers.size(), samplers.begin(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ASSERT_DX(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
    ASSERT_DX(obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&rootData.obj)));
    return root;
}

Pipeline Gpu::createPipeline(Root root, const char* shader) {
    Pipeline pipeline = pipelines.add();
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

Pipeline Gpu::createPipeline(Root root, PsoGraphicsDesc desc) {
    Pipeline pipeline = pipelines.add();
    PipelineData& pipelineData = pipelines[pipeline];
    Shader vShader(desc.vs);
    Shader pShader(desc.ps);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature         = get(root),
        .VS                     = vShader.bytecode(),
        .PS                     = pShader.bytecode(),
        .BlendState             = desc.blendState,
        .SampleMask             = desc.sampleMask,
        .RasterizerState        = desc.rasterizerState,
        .DepthStencilState      = desc.depthStencilState,
        .InputLayout            = {
                                    desc.inputLayout.data(),
                                    static_cast<UINT>(desc.inputLayout.size()),
                                  },
        .IBStripCutValue        = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        .PrimitiveTopologyType  = desc.topology,
        .NumRenderTargets       = desc.numRenderTargets,
        .RTVFormats             = {
                                    desc.rtvFormats[0],
                                    desc.rtvFormats[1],
                                    desc.rtvFormats[2],
                                    desc.rtvFormats[3],
                                    desc.rtvFormats[4],
                                    desc.rtvFormats[5],
                                    desc.rtvFormats[6],
                                    desc.rtvFormats[7],
                                  },
        .DSVFormat              = desc.dsvFormat,
        .SampleDesc             = desc.sampleDesc,
        .NodeMask               = 0,
        .CachedPSO              = { nullptr, 0 },
    };
    ASSERT_DX(obj->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineData.obj)));
    return pipeline;
}




Swapchain Gpu::createSwapchain(Queue& queue, HWND hwnd, uint32_t w, uint32_t h, UINT frameCount) {
    Swapchain sw;
    sw.idx = swapchains.size();
    swapchains.push_back(SwapchainData{});
    SwapchainData& swapchainData = swapchains.back();
    
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

    for (UINT i = 0; i < frameCount; i++) {
        SwapTarget& target = swapchainData.targets[i];
        
        target.texture = textures.add();
        TextureData& textureData = textures[target.texture];
        ASSERT_DX(swapchainData.obj->GetBuffer(i, IID_PPV_ARGS(&textureData.obj)));
        // D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {
        //     .Format         = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        //     .ViewDimension  = D3D12_RTV_DIMENSION_TEXTURE2D,
        //     .Texture2D = {
        //         .MipSlice   = 0,
        //         .PlaneSlice = 0,
        //     }
        // };
        target.rtv = nextRtv(0, target.texture);
    }
    return sw;
}

void Gpu::present(Swapchain sw, bool vsync) {
    SwapchainData& swapchainData = swapchains[sw];
    if (vsync) {
        ASSERT_DX(swapchainData.obj->Present(1, 0));
    } else {
        ASSERT_DX(swapchainData.obj->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    }
}

SwapTarget Gpu::next(Swapchain sw) {
    SwapchainData& swapchainData = swapchains[sw];
    swapchainData.frameIdx = swapchainData.obj->GetCurrentBackBufferIndex();
    return swapchainData.targets[swapchainData.frameIdx];
}






























Buffer Gpu::createBuffer(
    uint32_t              size,
    D3D12_HEAP_TYPE       heap,
    D3D12_RESOURCE_STATES state,
    D3D12_RESOURCE_FLAGS  flags
) {
    Buffer buffer = buffers.add();
    BufferData& bufferData = buffers[buffer];

    bufferData.size  = size;
    bufferData.heap  = heap;
    bufferData.state = state;
    bufferData.flags = flags;

    D3D12_HEAP_PROPERTIES props = {
        .Type                   = heap,
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
        .Flags              = flags,
    };
    ASSERT_DX(obj->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &resDesc, state, nullptr, IID_PPV_ARGS(&bufferData.obj)));

    if (heap == D3D12_HEAP_TYPE_UPLOAD) {
        D3D12_RANGE readRange = { 0, 0 }; // We do not intend to read from this resource on the CPU.
        ASSERT_DX(bufferData.obj->Map(0, &readRange, reinterpret_cast<void**>(&bufferData.data)));
    }
    return buffer;
}

void Gpu::write(Buffer buffer, uint32_t offset, uint32_t size, const uint8_t* data) {
    BufferData& bufferData = buffers[buffer];
    ASSERT(bufferData.heap == D3D12_HEAP_TYPE_UPLOAD);
    ASSERT(bufferData.size >= offset + size);
    memcpy(bufferData.data + offset, data, size);
}

void Gpu::read(Buffer buffer, const std::function<void(uint8_t*)>& f) {
    BufferData& bufferData = buffers[buffer];
    ASSERT(bufferData.heap == D3D12_HEAP_TYPE_READBACK);
    uint8_t* data = nullptr;
    bufferData.obj->Map(0, nullptr, reinterpret_cast<void**>(&data));
    f(data);
    bufferData.obj->Unmap(0, nullptr);
}

D3D12_GPU_VIRTUAL_ADDRESS Gpu::addr(Buffer buffer) {
    BufferData& bufferData = buffers[buffer];
    return bufferData.obj->GetGPUVirtualAddress();
}








Texture Gpu::createTexture(const uint8_t* data, uint32_t size, std::vector<D3D12_SUBRESOURCE_DATA>& subresources) {
    Texture texture = textures.add();
    TextureData& textureData = textures[texture];
    ASSERT_DX(LoadDDSTextureFromMemory(obj.Get(), data, size, textureData.obj.GetAddressOf(), subresources));
    return texture;
}


Texture Gpu::createTexture(TextureDesc& desc) {
    Texture texture = textures.add();
    TextureData& textureData = textures[texture];

    textureData.state = desc.state;
    D3D12_HEAP_PROPERTIES props = {
        .Type                   = desc.heap,
        .CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask       = 1,
        .VisibleNodeMask        = 1,
    };
    D3D12_RESOURCE_DESC resDesc = {
        .Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment          = 0,
        .Width              = desc.width,
        .Height             = desc.height,
        .DepthOrArraySize   = 1,
        .MipLevels          = 1,
        .Format             = desc.format,
        .SampleDesc         = { 1, 0 },
        .Layout             = desc.layout,
        .Flags              = desc.flags,
    };
    ASSERT_DX(obj->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &resDesc, desc.state, desc.hasClearValue ? &desc.clearValue : nullptr, IID_PPV_ARGS(&textureData.obj)));
    textureData.obj->SetName(L"Default texture");
    return texture;
}











ID3D12GraphicsCommandList* Gpu::get(Command command) {
    return commands[command].cmdList.Get();
}

ID3D12RootSignature* Gpu::get(Root root) {
    return roots[root].obj.Get();
}

ID3D12PipelineState* Gpu::get(Pipeline pipeline) {
    return pipelines[pipeline].obj.Get();
}

ID3D12Resource* Gpu::get(Buffer buffer) {
    return buffers[buffer].obj.Get();
}

ID3D12Resource* Gpu::get(Texture texture) {
    return textures[texture].obj.Get();
}





































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


}