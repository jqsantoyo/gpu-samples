#include <rendererInterface/renderer.h>
#include <gpu/gpu.h>
#include <app/app.h>
#include <directxmath.h>

using namespace DirectX;
namespace gpu {


struct ObjectData {
    XMFLOAT4X4 mvp;
};




class Renderer : public IRenderer {
public:
    Renderer(bool vulkan) : vulkan(vulkan) {}

    bool init(void* window, uint32_t width, uint32_t height) {
        viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0, 1 };
        scissorRect = { 0, 0, width, height };
        uint32_t frameCount = 2;
        uint32_t objectMemory = sizeof(ObjectData);
        uint32_t alignedObjectMemory = align256(objectMemory);
        uint32_t maxFrameMemory = 100 * alignedObjectMemory;

        
        gpu       = createGpu();
        queue     = gpu->createQueue();
        swapchain = gpu->createSwapchain(queue, window, width, height, frameCount);
        frameControl.init(gpu.get(), 2, maxFrameMemory);

        PsoGraphicsDesc psoFillDesc = {
            .vs                 = "shaders_v",
            .ps                 = "shaders_p",
            .fillMode           = FillMode::Solid,
            .cullMode           = CullMode::None,
            .enableDepth        = true,
            .topologyType       = PrimitiveTopologyType::Triangle,
            .inputElements      = {
                { "POSITION", 0, Format::RGB32f, 0, 0 },
                { "COLOR",    0, Format::RGB32f, 1, 0 }
            },
            .renderTargets      = { { Format::RGBA8un, true } },
            .dsvFormat          = Format::D32,
            .samples            = 1,
        };

        PsoGraphicsDesc psoWireDesc = {
            .vs                 = "shadersWire_v",
            .ps                 = "shadersWire_p",
            .fillMode           = FillMode::Wireframe,
            .cullMode           = CullMode::None,
            .enableDepth        = true,
            .topologyType       = PrimitiveTopologyType::Triangle,
            .inputElements      = {
                { "POSITION", 0, Format::RGB32f, 0, 0 },
            },
            .renderTargets      = { { Format::RGBA8un, false } },
            .dsvFormat          = Format::D32,
            .samples            = 1,
        };


        root         = gpu->createRoot({
            RootParam::binding(RootBinding::Constant, 0)
        }, {});
        psoFill      = gpu->createPipeline(root, psoFillDesc);
        psoWire      = gpu->createPipeline(root, psoWireDesc);
        depthTexture = gpu->createTexture2("depth", TextureUsage::DepthStencil, Format::D32, { width, height, 1 }, 1, 1, { 1, 0u });
        depthView    = gpu->createDepthView(depthTexture);
        meshRegistry.init(gpu.get(), 100);

        reset();
        return true;
    }

    void terminate() {
        gpu->wait(queue);
    }

    void reset() {
        wait();
        meshRegistry.reset();
    }

    void wait() {
        gpu->wait(queue);
    }

    bool resize(int width, int height) {
        return 1;
    }

    bool render(const RenderView& view) {
        static uint64_t frameIdx = 0;
        beginEvent("Render %llu", frameIdx);
        
        
        XMVECTOR posVec      = XMVectorSet(view.camera->pos.x,    view.camera->pos.y,    view.camera->pos.z,    1.0f);
        XMVECTOR targetVec   = XMVectorSet(view.camera->target.x, view.camera->target.y, view.camera->target.z, 1.0f);
        XMVECTOR upVec       = XMVectorSet(view.camera->up.x,     view.camera->up.y,     view.camera->up.z,     1.0f);
        XMMATRIX viewMat     = XMMatrixLookAtLH(posVec, targetVec, upVec);
        XMMATRIX projMat     = XMMatrixPerspectiveFovLH(view.camera->fovY, view.camera->aspect, view.camera->nearZ, view.camera->farZ);
        XMMATRIX viewProjMat = viewMat * projMat;


        SwapTarget target = gpu->next(swapchain);

        Command cmd = frameControl.next();
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
                MeshData& m = meshRegistry.get({model.meshId});
    
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
                MeshData& m = meshRegistry.get({model.meshId});

                
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
        endEvent();
        frameIdx++;
        return 1;
    }

    Buffer create(const char* name, uint32_t size, const uint8_t* data) {
        Buffer buffer = gpu->createBuffer(name, BufferUpload, size);
        gpu->write(buffer, 0, size, data);
        return buffer;
    }
    
    Mesh create(const MeshData& desc) {
        return meshRegistry.create(desc);
    }

    MaterialTexture create(const char* name, const uint8_t* data, uint32_t size) {
        return { -1 };
    }
    
    Material create(const char* name, MaterialDesc& desc) {
        return { -1 };
    }
    
    void destroy(Buffer buffer) {
        gpu->destroy(buffer);
    }
    
    void destroy(MaterialTexture materialTexture) {
    }
    
    void destroy(Material material) {
    }


private:
    bool                    vulkan;
    std::unique_ptr<IGpu>   gpu;
    Queue                   queue;
    Swapchain               swapchain;
    Root                    root;
    Pipeline                psoFill;
    Pipeline                psoWire;
    FrameControl            frameControl;
    MeshRegistry            meshRegistry;
    Texture                 depthTexture;
    TextureView             depthView;
    Viewport                viewport;
    Rect                    scissorRect;
};

std::unique_ptr<IRenderer> createRendererBasic(bool vulkan) {
    return std::make_unique<Renderer>(vulkan);
}
}

// 563