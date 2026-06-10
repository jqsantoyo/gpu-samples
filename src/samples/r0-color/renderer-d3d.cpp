#include <rendererInterface/renderer.h>
#include <gpuD3D/gpu.h>
#include <utils/utils.h>
#include <app/app.h>
#include <pix3.h>
#include <wrl.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>
#include <iostream>

using namespace DirectX;
using namespace Microsoft::WRL;
namespace gpu {


struct ObjectData {
    XMFLOAT4X4 mvp;
};




class RendererD3D : public IRenderer {
public:

    bool init(void* window, uint32_t width, uint32_t height) {
        auto hwnd = static_cast<HWND>(window);
        viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0, 1 };
        scissorRect = { 0, 0, long(width), long(height) };
        UINT frameCount = 2;
        uint32_t objectMemory = sizeof(ObjectData);
        uint32_t alignedObjectMemory = align256(objectMemory);
        uint32_t maxFrameMemory = 100 * alignedObjectMemory;

        
        gpu.init(frameCount, 1, 100);
        queue     = gpu.createQueue();
        swapchain = gpu.createSwapchain(queue, hwnd, width, height, frameCount);
        frameControl.init(gpu, 2, maxFrameMemory);


        // D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels = {
        //     .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        //     .SampleCount = 4,
        //     .Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
        //     .NumQualityLevels = 0,
        // };
        // GUARDHR(gpu->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)));
        // GUARD((qualityLevels.NumQualityLevels > 0));
        // DXGI_SAMPLE_DESC sampleDesc = { .Count = 4, .Quality = qualityLevels.NumQualityLevels - 1 }; 
        gpu::PsoGraphicsDesc psoFillDesc = {
            .vs                         = "shaders_v",
            .ps                         = "shaders_p",
            .blendState                 = gpu::noBlend(),
            .sampleMask                 = UINT_MAX,
            .rasterizerState            = gpu::defaultFill(),
            .depthStencilState          = gpu::defaultDepth(),
            .inputLayout                = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 1,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            },
            .topology      = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .numRenderTargets           = 1,
            .rtvFormats                 = { DXGI_FORMAT_R8G8B8A8_UNORM },
            .dsvFormat                  = DXGI_FORMAT_D32_FLOAT,
            .sampleDesc                 = { .Count = 1, .Quality = 0 },
        };

        gpu::PsoGraphicsDesc psoWireDesc = {
            .vs                 = "shadersWire_v",
            .ps                 = "shadersWire_p",
            .blendState         = gpu::noBlend(),
            .sampleMask         = UINT_MAX,
            .rasterizerState    = gpu::defaultWire(),
            .depthStencilState  = gpu::defaultDepth(),
            .inputLayout        = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            },
            .topology           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .numRenderTargets   = 1,
            .rtvFormats         = { DXGI_FORMAT_R8G8B8A8_UNORM },
            .dsvFormat          = DXGI_FORMAT_D32_FLOAT,
            .sampleDesc         = { .Count = 1, .Quality = 0 },
        };


        root    = gpu.createRoot({ gpu::rootCbv(0) }, {});
        psoFill = gpu.createPipeline(root, psoFillDesc);
        psoWire = gpu.createPipeline(root, psoWireDesc);
        depthBuffer.init(&gpu, width, height);
        meshRegistry.init(&gpu);
        
        buffersBaseIdx = gpu.buffers.size();
        meshesBaseIdx  = meshRegistry.getMeshCount();

        reset();
        return true;
    }

    void terminate() {
        gpu.wait(queue);
    }

    void reset() {
        wait();
        // gpu.resetMain(1);
        gpu.buffers.resize(buffersBaseIdx);
        meshRegistry.reset(meshesBaseIdx);
    }

    void wait() {
        gpu.wait(queue);
    }

    bool resize(int width, int height) {
        return 1;
    }

    bool render(const RenderView& view) {
        static uint64_t frameIdx = 0;
        PIXBeginEvent(PIX_COLOR_DEFAULT, "Render %llu", frameIdx);
        
        
        XMVECTOR posVec      = XMVectorSet(view.camera->pos.x,    view.camera->pos.y,    view.camera->pos.z,    1.0f);
        XMVECTOR targetVec   = XMVectorSet(view.camera->target.x, view.camera->target.y, view.camera->target.z, 1.0f);
        XMVECTOR upVec       = XMVectorSet(view.camera->up.x,     view.camera->up.y,     view.camera->up.z,     1.0f);
        XMMATRIX viewMat     = XMMatrixLookAtLH(posVec, targetVec, upVec);
        XMMATRIX projMat     = XMMatrixPerspectiveFovLH(view.camera->fovY, view.camera->aspect, view.camera->nearZ, view.camera->farZ);
        XMMATRIX viewProjMat = viewMat * projMat;


        gpu::SwapTarget target = gpu.next(swapchain);

        gpu::Command cmd = frameControl.next();
        gpu.wait(queue, cmd);
        gpu.begin(cmd);

        ID3D12GraphicsCommandList* cmdList = gpu.get(cmd);
        gpu.viewport (cmd, viewport);
        gpu.scissor  (cmd, scissorRect);
        gpu.barrier  (cmd, target.texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
        gpu.targets  (cmd, target.rtv, depthBuffer.dsv);
        gpu.clear    (cmd, target.rtv, view.clearColor);
        gpu.clear    (cmd, depthBuffer.dsv,  1, 0);
        gpu.heaps    (cmd, gpu.mainHeap.get());
        
        if (view.fillMode == Fill || view.fillMode == FillWire) {
            // PIXBeginEvent(queue.obj.Get(), PIX_COLOR_DEFAULT, "Fill %llu", frameIdx);
            gpu.pipeline          (cmd, psoFill);
            gpu.graphicsRoot (cmd, root);
            gpu.topology     (cmd, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                Mesh& m = meshRegistry.getMesh(model.meshId);
    
                ObjectData objectData = {};
                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&transform));
                XMMATRIX mvpMat = modelMat * viewMat * projMat;
                XMStoreFloat4x4(&objectData.mvp, mvpMat);
                // XMStoreFloat4x4(&objectData.mvp, XMMatrixTranspose(mvp)));
                
                gpu.graphicsCbv  (cmd, 0, objectData);
                gpu.vertexBuffer (cmd, 0, m.positionView);
                gpu.vertexBuffer (cmd, 1, m.colorView);
                if (m.indicesView.SizeInBytes != 0) {
                    gpu.indexBuffer(cmd, m.indicesView);
                    gpu.drawIndexed(cmd, m.vCount, 1, 0, 0, 0);
                } else {
                    gpu.draw(cmd, m.vCount, 1, 0, 0);
                }
            }
            // PIXEndEvent(queue.obj.Get());
        }

        if (view.fillMode == Wire || view.fillMode == FillWire) {
            // PIXBeginEvent(queue.obj.Get(), PIX_COLOR_DEFAULT, "Wire %llu", frameIdx);
            gpu.pipeline          (cmd, psoWire);
            gpu.graphicsRoot (cmd, root);
            gpu.topology     (cmd, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                Mesh& m = meshRegistry.getMesh(model.meshId);

                
                ObjectData objectData = {};
                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&transform));
                XMMATRIX mvpMat = modelMat * viewMat * projMat;
                XMStoreFloat4x4(&objectData.mvp, mvpMat);
                // XMStoreFloat4x4(&objectData.mvp, XMMatrixTranspose(mvp)));
                
                gpu.graphicsCbv  (cmd, 0, objectData);
                gpu.vertexBuffer (cmd, 0, m.positionView);
                gpu.vertexBuffer (cmd, 1, m.colorView);
                if (m.indicesView.SizeInBytes != 0) {
                    gpu.indexBuffer(cmd, m.indicesView);
                    gpu.drawIndexed(cmd, m.vCount, 1, 0, 0, 0);
                } else {
                    gpu.draw(cmd, m.vCount, 1, 0, 0);
                }
            }
            // PIXEndEvent(queue.obj.Get());
        }


        gpu.barrier(cmd, target.texture, D3D12_RESOURCE_STATE_PRESENT);
        gpu.end(cmd);
        gpu.execute(queue, cmd);
        gpu.present(swapchain, true);
        gpu.signal(queue, cmd);
        PIXEndEvent();
        frameIdx++;
        return 1;
    }
    
    void trs2Transform(int count, const Trs* trs, mat4* transforms) {
        for (int i = 0; i < count; i++, trs++, transforms++) {
            XMVECTOR q = XMVectorSet(trs->rotation.x, trs->rotation.y, -trs->rotation.z, -trs->rotation.w);
            q = XMQuaternionNormalize(q);
            XMMATRIX S = XMMatrixScaling(trs->scale.x, trs->scale.y, -trs->scale.z);
            XMMATRIX R = XMMatrixRotationQuaternion(q);
            XMMATRIX T = XMMatrixTranslation(trs->position.x, trs->position.y, -trs->position.z);
            XMMATRIX transformMat = S * R * T;
            XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(transforms), transformMat);
        }
    }
    

    int addBuffer(const BufferDesc& desc) {
        gpu::Buffer buffer = gpu.createBuffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        gpu.write(buffer, desc.offset, desc.size, desc.data);
        return buffer.idx;
    }
    
    int addMesh(const MeshDesc& desc) {
        return meshRegistry.addMesh(desc);
    }

    int getBufferCount() {
        return gpu.buffers.size();
    }

    int getMeshCount() {
        return meshRegistry.getMeshCount();
    }

private:
    gpu::Gpu                 gpu;
    gpu::Queue               queue;
    gpu::Swapchain           swapchain;
    gpu::Root                root;
    gpu::Pipeline            psoFill;
    gpu::Pipeline            psoWire;
    FrameControl   frameControl;
    MeshRegistry   meshRegistry;
    DepthBuffer    depthBuffer;
    D3D12_VIEWPORT      viewport;
    D3D12_RECT          scissorRect;
    int                 rtvBaseIdx;       // Tracks beginning of user-level rtv descriptors
    int                 dsvBaseIdx;       // Tracks beginning of user-level dsv descriptors
    int                 cbvBaseIdx;       // Tracks beginning of user-level cbv descriptors
    int                 buffersBaseIdx;   // Tracks beginning of user-level buffers
    int                 meshesBaseIdx;    // Tracks beginning of user-level meshes
    int                 texturesBaseIdx;  // Tracks beginning of user-level textures
    int                 materialsBaseIdx; // Tracks beginning of user-level materials
};

std::unique_ptr<IRenderer> createRendererD3D() {
    return std::make_unique<RendererD3D>();
}
}

// 563