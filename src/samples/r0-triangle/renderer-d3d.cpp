#include <utils/utils.h>
#include <utils/app.h>
#include <utils/renderer.h>
#include <utilsD3D/utilsD3D.h>
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <pix3.h>
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
        D3D12_COMMAND_QUEUE_DESC queueDesc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        };

        GUARD(factory.init());
        factory.print();
        GUARD(factory.select());
        GUARD(device.init(factory.getSelected(), frameCount, 0, 0));
        GUARD(queue.init(device, queueDesc));
        GUARD(swapchain.init(factory, device, queue, hwnd, screenWidth, screenHeight, frameCount));
        GUARD(frameControl.init(device, &queue, 1));
        GUARD(rootSignature.initVoid(device));
        GUARD(pso.init(device, rootSignature));

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
            .color      = { sizeof(float) * 3 * 3, sizeof(float) * 3 * 3 },
        };
        meshId = meshControl.addMesh(meshDesc);

        queue.wait();
        return true;
    }

    void terminate() {
        queue.wait();
    }

    bool resize(int width, int height) {
        return true;
    }

    bool render(const Color& clearColor, const std::vector<RenderItem>& items) {
        static uint64_t frameIdx = 0;
        PIXBeginEvent(PIX_COLOR_DEFAULT, "Render %llu", frameIdx);
        RenderTarget target = swapchain.next();
        auto barr0 = CD3DX12_RESOURCE_BARRIER::Transition(target.resource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto barr1 = CD3DX12_RESOURCE_BARRIER::Transition(target.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        frameControl.begin(pso.obj.Get());
        frameControl.cmdList->SetGraphicsRootSignature(rootSignature.obj.Get());
        frameControl.cmdList->RSSetViewports(1, &viewport);
        frameControl.cmdList->RSSetScissorRects(1, &scissorRect);
        frameControl.cmdList->ResourceBarrier(1, &barr0); // Use back buffer as a render target.
        frameControl.cmdList->OMSetRenderTargets(1, &target.view, FALSE, nullptr);
        frameControl.cmdList->ClearRenderTargetView(target.view, clearColor.v, 0, nullptr);
        frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        Mesh& mesh = meshControl.getMesh(meshId);
        frameControl.cmdList->IASetVertexBuffers(0, 1, &mesh.positionView);
        frameControl.cmdList->IASetVertexBuffers(1, 1, &mesh.colorView);
        frameControl.cmdList->DrawInstanced(mesh.vCount, 1, 0, 0);

        frameControl.cmdList->ResourceBarrier(1, &barr1); // Use back buffer to present.
        GUARD(frameControl.execute());
        GUARD(swapchain.present(false));
        GUARD(frameControl.end());
        PIXEndEvent();
        frameIdx++;
        return 1;
    }

private:
    Factory                             factory;
    Device                              device;
    Swapchain                           swapchain;
    Queue                               queue;
    FrameControl                        frameControl;
    MeshControl                         meshControl;
    RootSig                             rootSignature;
    PipelineBasic                       pso;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
    float                               screenAR;
    int                                 meshId;
};

std::unique_ptr<IRenderer> createRendererD3D() {
    return std::make_unique<RendererD3D>();
}
}
