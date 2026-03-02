#include <renderer/renderer.h>
#include <app/app.h>
#include <utils/utils.h>
#include <utilsD3D/utilsD3D.h>
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
#include <wrl.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>

using namespace DirectX;
using namespace Microsoft::WRL;
namespace gpu {


class RendererD3D : public IRenderer {
public:
    bool init(void* window, uint32_t screenWidth, uint32_t screenHeight) {
        auto hwnd = static_cast<HWND>(window);
        screenAR = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
        viewport = { 0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight) };
        scissorRect = { 0, 0, long(screenWidth), long(screenHeight) };


        UINT frameCount = 2;
        GUARD(factory.init());
        Adapter* adapter = factory.select();
        GUARD(adapter != nullptr);
        GUARD(device.init(adapter, frameCount, 0, 0));

        GUARDHR(device.obj->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
        GUARDHR(device.obj->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));
        GUARDHR(cmdList.Get()->Close());
        GUARDHR(device.obj->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr) {
            GUARDHR(HRESULT_FROM_WIN32(GetLastError()));
        }
        fenceValue = 1;


        GUARD(swapchain.init(factory, device, hwnd, screenWidth, screenHeight, frameCount));
        GUARD(frameControl.init(device, &swapchain, 1));

        float vertices[] = {
             0.0f,   0.25f * screenAR, 0.0f,
             0.25f, -0.25f * screenAR, 0.0f,
            -0.25f, -0.25f * screenAR, 0.0f,
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f
        };
        BufferDesc bufferDesc = { 0, sizeof(vertices), reinterpret_cast<uint8_t*>(vertices) };
        int bufferId = meshControl.addBuffer(device, bufferDesc);
        MeshDesc meshDesc = {
            .bufferId   = bufferId,
            .vCount     = 3,
            .indices    = { 0, 0 },
            .position   = { 0, sizeof(float) * 3 * 3 },
            .normal     = { 0, 0 },
            .uv         = { 0, 0 },
            .color      = { sizeof(float) * 3 * 3, sizeof(float) * 4 * 3 },
        };
        meshId = meshControl.addMesh(meshDesc);





        ////////////////////////////////////////////////////////////////////////////////////////////
        // PSOs
        ComPtr<ID3DBlob>            sig;
        ComPtr<ID3DBlob>            error;
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        GUARDHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
        GUARDHR(device.obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

        Shader vShader;
        Shader pShader;
        std::string shaderDir = "03-triangle-shaders";
        GUARD(vShader.load(shaderDir, "shaders_v.dxil"));
        GUARD(pShader.load(shaderDir, "shaders_p.dxil"));

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
            .pRootSignature         = rootSignature.Get(),
            .VS                     = vShader.bytecode,
            .PS                     = pShader.bytecode,
            .BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask             = UINT_MAX,
            .RasterizerState        = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState      = { .DepthEnable = FALSE, .StencilEnable = FALSE, },
            .InputLayout            = { inputElementDescs, _countof(inputElementDescs) },
            .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets       = 1,
            .RTVFormats             = { DXGI_FORMAT_R8G8B8A8_UNORM },
            .SampleDesc             = { .Count = 1},
        };
        GUARDHR(device.obj->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

        

        // Wait for assets to upload to the GPU.
        GUARD(waitForPreviousFrame());
        return 1;
    }

    void terminate() {
        waitForPreviousFrame();
        CloseHandle(fenceEvent);
    }

    bool resize(int width, int height) {
        return true;
    }

    bool render(const Color& clearColor, const std::vector<RenderItem>& items) {
        RenderTarget target = swapchain.next();
        auto barr0 = CD3DX12_RESOURCE_BARRIER::Transition(target.resource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto barr1 = CD3DX12_RESOURCE_BARRIER::Transition(target.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        frameControl.begin(pso.Get());
        frameControl.cmdList->SetGraphicsRootSignature(rootSignature.Get());
        frameControl.cmdList->RSSetViewports(1, &viewport);
        frameControl.cmdList->RSSetScissorRects(1, &scissorRect);
        frameControl.cmdList->ResourceBarrier(1, &barr0); // Use back buffer as a render target.
        frameControl.cmdList->OMSetRenderTargets(1, &target.view, FALSE, nullptr);
        frameControl.cmdList->ClearRenderTargetView(target.view, clearColor.v, 0, nullptr);
        frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        Mesh& mesh = meshControl.meshes[meshId];
        frameControl.cmdList->IASetVertexBuffers(0, 1, &mesh.positionView);
        frameControl.cmdList->IASetVertexBuffers(1, 1, &mesh.colorView);
        frameControl.cmdList->DrawInstanced(mesh.vCount, 1, 0, 0);

        frameControl.cmdList->ResourceBarrier(1, &barr1); // Use back buffer to present.
        frameControl.end();
        return 1;
    }
    
    int waitForPreviousFrame() {
        const UINT64 f = fenceValue;
        GUARDHR(device.cmdQueue->Signal(fence.Get(), f));
        fenceValue++;
        if (fence->GetCompletedValue() < f) {
            GUARDHR(fence->SetEventOnCompletion(f, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
        }
        return 1;
    }

private:
    Factory                             factory;
    Device                              device;
    Swapchain                           swapchain;
    FrameControl                        frameControl;
    MeshControl                         meshControl;
    ComPtr<ID3D12CommandAllocator>      cmdAllocator;
    ComPtr<ID3D12GraphicsCommandList>   cmdList;
    ComPtr<ID3D12Fence>                 fence;
    HANDLE                              fenceEvent;
    UINT64                              fenceValue;
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         pso;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
    float                               screenAR;
    int                                 meshId;
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererD3D>();
}
}
