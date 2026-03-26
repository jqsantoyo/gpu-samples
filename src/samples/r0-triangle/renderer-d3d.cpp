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
        viewport = { 0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight) };
        scissorRect = { 0, 0, long(screenWidth), long(screenHeight) };
        UINT frameCount = 2;

        GUARD(factory.init());
        factory.print();
        GUARD(factory.select());
        GUARD(device.init(factory.getSelected(), frameCount, 0, 0));
        GUARD(queue.init(device, D3D12_COMMAND_LIST_TYPE_DIRECT));
        GUARD(swapchain.init(factory, device, queue, hwnd, screenWidth, screenHeight, frameCount));
        GUARD(frameControl.init(device, &queue, 1));
        GUARD(rootSignature.initVoid(device));
        GUARD(pso.init(device, rootSignature));
        queue.wait();
        return true;
    }

    void terminate() {
        queue.wait();
    }

    void wait() {
        queue.wait();
    }

    bool resize(int width, int height) {
        return true;
    }

    bool render(const Color& clearColor, const std::vector<RenderItem>& items) {
        static uint64_t frameIdx = 0;
        PIXBeginEvent(PIX_COLOR_DEFAULT, "Render %llu", frameIdx);
        RenderTarget target = swapchain.next();

        frameControl.begin(pso.obj.Get());
        frameControl.cmdList->SetGraphicsRootSignature(rootSignature.obj.Get());
        frameControl.cmdList->RSSetViewports(1, &viewport);
        frameControl.cmdList->RSSetScissorRects(1, &scissorRect);
        frameControl.barrier(target.resource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        frameControl.cmdList->OMSetRenderTargets(1, &target.view, FALSE, nullptr);
        frameControl.cmdList->ClearRenderTargetView(target.view, clearColor.v, 0, nullptr);
        frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        for (int i = 0; i < items.size(); i++) {
            const RenderItem& item = items[i];
            int meshId = item.meshId;
            Mesh& m = meshRegistry.getMesh(meshId);
            frameControl.cmdList->IASetVertexBuffers(0, 1, &m.positionView);
            frameControl.cmdList->IASetVertexBuffers(1, 1, &m.colorView);
            frameControl.cmdList->DrawInstanced(m.vCount, 1, 0, 0);
        }

        frameControl.barrier(target.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        GUARD(frameControl.execute());
        GUARD(swapchain.present(false));
        GUARD(frameControl.end());
        PIXEndEvent();
        frameIdx++;
        return 1;
    }

    int addBuffer(const BufferDesc& desc) {
        return meshRegistry.addBuffer(device, desc);
    }
    
    int addMesh(const MeshDesc& desc) {
        return meshRegistry.addMesh(desc);
    }

private:
    Factory                             factory;
    Device                              device;
    Swapchain                           swapchain;
    Queue                               queue;
    FrameControl                        frameControl;
    MeshRegistry                        meshRegistry;
    RootSig                             rootSignature;
    PipelineBasic                       pso;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
    int                                 meshId;
};

std::unique_ptr<IRenderer> createRendererD3D() {
    return std::make_unique<RendererD3D>();
}
}
