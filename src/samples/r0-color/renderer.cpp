#include <renderer/renderer.h>
#include <renderer/rendererBase.h>
#include <gpu/gpu.h>
#include <app/app.h>
#include <directxmath.h>

using namespace DirectX;
namespace gpu {


struct ObjectData {
    XMFLOAT4X4 mvp;
};




class Renderer : public RendererBase {
public:

    void init2() {
        uint32_t width  = static_cast<uint32_t>(viewport.width);
        uint32_t height = static_cast<uint32_t>(viewport.height);
        uint32_t frameCount = 2;
        uint32_t objectMemory = sizeof(ObjectData);
        uint32_t alignedObjectMemory = align256(objectMemory);
        uint32_t maxFrameMemory = 100 * alignedObjectMemory;


        PsoGraphicsDesc psoFillDesc = {
            .vs                 = "shaders_v",
            .ps                 = "shaders_p",
            .inputElements      = {
                { "POSITION", 0, Format::RGB32f, 0, 0 },
                { "COLOR",    0, Format::RGB32f, 1, 0 }
            },
        };

        PsoGraphicsDesc psoWireDesc = {
            .vs                 = "shadersWire_v",
            .ps                 = "shadersWire_p",
            .fillMode           = FillMode::Wireframe,
            .inputElements      = {
                { "POSITION", 0, Format::RGB32f, 0, 0 },
            },
        };

        root         = gpu->createRoot({
            RootParam::binding(RootBinding::Constant, 0)
        }, {});
        psoFill      = gpu->createPipeline(root, psoFillDesc);
        psoWire      = gpu->createPipeline(root, psoWireDesc);
        depthTexture = gpu->createTexture2("depth", TextureUsage::DepthStencil, Format::D32, { width, height, 1 }, 1, 1, { 1, 0u });
        depthView    = gpu->createDepthView(depthTexture);
    }

    void terminate2() {
    }

    void reset2() {
    }

    void resize2(int width, int height) {
    }

    void render2(SwapTarget target, Command cmd, const RenderView& view) {
        
        
        XMVECTOR posVec      = XMVectorSet(view.camera->pos.x,    view.camera->pos.y,    view.camera->pos.z,    1.0f);
        XMVECTOR targetVec   = XMVectorSet(view.camera->target.x, view.camera->target.y, view.camera->target.z, 1.0f);
        XMVECTOR upVec       = XMVectorSet(view.camera->up.x,     view.camera->up.y,     view.camera->up.z,     1.0f);
        XMMATRIX viewMat     = XMMatrixLookAtLH(posVec, targetVec, upVec);
        XMMATRIX projMat     = XMMatrixPerspectiveFovLH(view.camera->fovY, view.camera->aspect, view.camera->nearZ, view.camera->farZ);
        XMMATRIX viewProjMat = viewMat * projMat;

        gpu->wait(queue, cmd);
        gpu->begin(cmd);

        gpu->viewport (cmd, viewport);
        gpu->scissor  (cmd, scissorRect);
        gpu->barrier  (cmd, target.texture, State::RenderTarget);
        gpu->targets  (cmd, target.renderView, depthView);
        gpu->clear    (cmd, target.renderView, view.clearColor, depthView, 1, 0);
        
        if (view.fillMode == Fill || view.fillMode == FillWire) {
            // PIXBeginEvent(queue.obj.Get(), PIX_COLOR_DEFAULT, "Fill %llu", frameIdx);
            gpu->pipeline     (cmd, psoFill);
            gpu->graphicsRoot (cmd, root);
            gpu->topology     (cmd, PrimitiveTopology::TriangleList);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                MeshData& m = meshes[model.meshId];
    
                ObjectData objectData = {};
                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&transform));
                XMMATRIX mvpMat = modelMat * viewMat * projMat;
                XMStoreFloat4x4(&objectData.mvp, mvpMat);
                // XMStoreFloat4x4(&objectData.mvp, XMMatrixTranspose(mvp)));
                
                gpu->graphicsCbv  (cmd, 0, objectData);
                gpu->vertexBuffer (cmd, 0, m.position);
                gpu->vertexBuffer (cmd, 1, m.color);
                if (m.indices.size != 0) {
                    gpu->indexBuffer(cmd, m.indices);
                    gpu->drawIndexed(cmd, m.vCount, 1, 0, 0, 0);
                } else {
                    gpu->draw(cmd, m.vCount, 1, 0, 0);
                }
            }
            // PIXEndEvent(queue.obj.Get());
        }

        if (view.fillMode == Wire || view.fillMode == FillWire) {
            // PIXBeginEvent(queue.obj.Get(), PIX_COLOR_DEFAULT, "Wire %llu", frameIdx);
            gpu->pipeline     (cmd, psoWire);
            gpu->graphicsRoot (cmd, root);
            gpu->topology     (cmd, PrimitiveTopology::TriangleList);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                MeshData& m = meshes[model.meshId];

                
                ObjectData objectData = {};
                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&transform));
                XMMATRIX mvpMat = modelMat * viewMat * projMat;
                XMStoreFloat4x4(&objectData.mvp, mvpMat);
                // XMStoreFloat4x4(&objectData.mvp, XMMatrixTranspose(mvp)));
                
                gpu->graphicsCbv  (cmd, 0, objectData);
                gpu->vertexBuffer (cmd, 0, m.position);
                gpu->vertexBuffer (cmd, 1, m.color);
                if (m.indices.size != 0) {
                    gpu->indexBuffer(cmd, m.indices);
                    gpu->drawIndexed(cmd, m.vCount, 1, 0, 0, 0);
                } else {
                    gpu->draw(cmd, m.vCount, 1, 0, 0);
                }
            }
            // PIXEndEvent(queue.obj.Get());
        }


        gpu->barrier(cmd, target.texture, State::Present);
        gpu->end(cmd);
        gpu->execute(queue, cmd);
        gpu->present(swapchain, false);
        gpu->signal(queue, cmd);
    }


private:
    Root                    root;
    Pipeline                psoFill;
    Pipeline                psoWire;
    Texture                 depthTexture;
    TextureView             depthView;
};

std::unique_ptr<IRenderer> createRendererBasic() {
    return std::make_unique<Renderer>();
}
}

// 563