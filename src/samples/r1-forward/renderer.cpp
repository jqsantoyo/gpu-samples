#include <rendererInterface/renderer.h>
#include <gpu/gpu.h>
#include <app/app.h>
#include <directxmath.h>

using namespace DirectX;

namespace gpu {



struct ObjectData {
    XMFLOAT4X4 m;
};

struct PassData {
    XMFLOAT4X4  view;
    XMFLOAT4X4  proj;
    XMFLOAT4X4  viewProj;
    vec3        eye;
    float       pad0;
    XMFLOAT4X4  lightViewProj;
    Light       light[12];
    vec2        screenSize;
    float       deltaTime;
    float       absoluteTime;
    int         shadowMap;
};



class Renderer : public IRenderer {
public:
    Renderer(bool vulkan) : vulkan(vulkan) {}

    bool init(void* window, uint32_t width, uint32_t height) {
        viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0, 1 };
        scissorRect = { 0, 0, width, height };
        uint32_t frameCount = 2;
        uint32_t materialData = align256(sizeof(MaterialData));
        uint32_t passData = align256(sizeof(PassData));
        uint32_t objectMemory = sizeof(ObjectData);
        uint32_t alignedObjectMemory = align256(objectMemory);
        uint32_t maxFrameMemory = passData + 10 * materialData + 100 * alignedObjectMemory;

        gpu       = createGpu();
        queue     = gpu->createQueue();
        swapchain = gpu->createSwapchain(queue, window, width, height, frameCount);

        PsoGraphicsDesc pipelineDesc = {
            .vs                 = "shaders_v",
            .ps                 = "shaders_p",
            .fillMode           = FillMode::Solid,
            .cullMode           = CullMode::None,
            .enableDepth        = true,
            .topologyType       = PrimitiveTopologyType::Triangle,
            .inputElements      = {
                { "POSITION", 0, Format::RGB32f,  0, 0 },
                { "NORMAL",   0, Format::RGB32f,  1, 0 },
                { "UV",       0, Format::RG32f,   2, 0 },
                { "TANGENT",  0, Format::RGBA32f, 3, 0 },
            },
            .renderTargets      = { { Format::RGBA8un, true } },
            .dsvFormat          = Format::D32,
            .samples            = 1,
        };

        PsoGraphicsDesc pipelineWireDesc = {
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
        root = gpu->createRoot({
                RootParam::binding(RootBinding::Constant, 0),
                RootParam::binding(RootBinding::Constant, 1),
                RootParam::binding(RootBinding::Constant, 2),
                RootParam::table({
                    RootRange::range(RootBinding::Read, 10, 0)
                }),
            }, {
                { 0, Filter::Anisotropic },
                { 1, Filter::Linear      },
            }
        );
        pipeline     = gpu->createPipeline(root, pipelineDesc);
        pipelineWire = gpu->createPipeline(root, pipelineWireDesc);
        mainCommand  = gpu->createCommand();
        uploadBuffer = gpu->createBuffer("UploadBuffer", BufferUpload, 2048 * 2048 * 4);

        frameControl.init(gpu.get(), 2, maxFrameMemory);
        depthTexture = gpu->createTexture2("depth", TextureUsage::DepthStencil, Format::D32, { width, height, 1 }, 1, 1, { 1, 0u });
        depthView    = gpu->createDepthView(depthTexture);
        meshRegistry    .init(gpu.get(), 100);
        materialRegistry.init(gpu.get(), queue, mainCommand, uploadBuffer, 100, 50);
        shadow          .init(gpu.get(), root, 1024, 1024);

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
        SwapTarget target = gpu->next(swapchain);


        Light light0 = { { -5, 2, 5 },  2, {  1, -1, -1 }, 10, { 3.0f, 3.0f, 3.0f }, 1 };
        Light light1 = { { 5,  5, 0 },  2, { -1, -1,  0 }, 10, { 3.0f, 3.0f, 3.0f }, 3 };
        Light light2 = { { 0, 1, -5 },  2, {  0,  0,  1 }, 10, { 3.0f, 3.0f, 3.0f }, 4 };
        
        static float t = 0;
        t += .016;
        PassData passData = {
            .eye = view.camera->pos,
            .light = {
                light0,
                light1,
                light2,
            },
            .deltaTime = 0,
            .absoluteTime = t,
            .shadowMap = gpu->getTextureViewBind(shadow.readView),
        };

        {
            XMVECTOR posVec      = XMVectorSet(light1.position.x, light1.position.y, light1.position.z, 1.0f);
            XMVECTOR targetVec   = XMVectorSet(0, 0, 0, 1.0f);
            XMVECTOR upVec       = XMVectorSet(0, 1, 0,     1.0f);
            XMMATRIX viewMat     = XMMatrixLookAtLH(posVec, targetVec, upVec);
            XMMATRIX projMat     = XMMatrixPerspectiveFovLH(view.camera->fovY, view.camera->aspect, view.camera->nearZ, view.camera->farZ);
            XMMATRIX viewProjMat = viewMat * projMat;
            XMStoreFloat4x4(&passData.lightViewProj, viewProjMat);
        }

        XMVECTOR posVec      = XMVectorSet(view.camera->pos.x,    view.camera->pos.y,    view.camera->pos.z,    1.0f);
        XMVECTOR targetVec   = XMVectorSet(view.camera->target.x, view.camera->target.y, view.camera->target.z, 1.0f);
        XMVECTOR upVec       = XMVectorSet(view.camera->up.x,     view.camera->up.y,     view.camera->up.z,     1.0f);
        XMMATRIX viewMat     = XMMatrixLookAtLH(posVec, targetVec, upVec);
        XMMATRIX projMat     = XMMatrixPerspectiveFovLH(view.camera->fovY, view.camera->aspect, view.camera->nearZ, view.camera->farZ);
        XMMATRIX viewProjMat = viewMat * projMat;
        XMStoreFloat4x4(&passData.view,         viewMat);
        XMStoreFloat4x4(&passData.proj,         projMat);
        XMStoreFloat4x4(&passData.viewProj,     viewProjMat);
        
        Command cmd = frameControl.next();
        gpu->wait(queue, cmd);
        gpu->begin(cmd);
        gpu->bindHeap(cmd);

        if (view.enableShadows) {
            Viewport shadowViewport = { 0.0f, 0.0f, static_cast<float>(1024), static_cast<float>(1024), 0, 1 };
            Rect shadowScissor = { 0, 0, long(1024), long(1024) };
            gpu->viewport     (cmd, shadowViewport);
            gpu->scissor      (cmd, shadowScissor);
            gpu->barrier      (cmd, shadow.target, State::DepthWrite);
            gpu->targets      (cmd, {-1}, shadow.depthView);
            gpu->clear        (cmd, {-1}, {}, shadow.depthView, 1, 0);

            gpu->pipeline          (cmd, shadow.pipeline);
            gpu->graphicsRoot (cmd, root);
            gpu->graphicsCbv  (cmd, 2, passData);
            gpu->graphicsTable(cmd, 3, 0);
            gpu->topology(cmd, PrimitiveTopology::TriangleList);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                int meshId = model.meshId;
                int materialId = model.materialId;
                if (meshId < 0) {
                    continue;
                }
                MeshData& m = meshRegistry.get({meshId});

                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(transform.v));
                ObjectData objectData = {};
                XMStoreFloat4x4(&objectData.m, modelMat);
                
                gpu->graphicsCbv  (cmd, 0, objectData);
                gpu->indexBuffer  (cmd, m.indices);
                gpu->vertexBuffer (cmd, 0, m.position);
                gpu->drawIndexed  (cmd, m.vCount, 1, 0, 0, 0);
            }
            gpu->barrier(cmd, shadow.target, State::GenericRead);
        } else {
            gpu->barrier(cmd, shadow.target, State::DepthWrite);
            gpu->clear  (cmd, {-1}, {}, shadow.depthView, 1, 0);
            gpu->barrier(cmd, shadow.target, State::GenericRead);
        }


        gpu->viewport (cmd, viewport);
        gpu->scissor  (cmd, scissorRect);
        gpu->barrier  (cmd, target.texture, State::RenderTarget);
        gpu->targets  (cmd, target.renderView, depthView);
        gpu->clear    (cmd, target.renderView, view.clearColor, depthView, 1, 0);

        if (view.fillMode == Fill || view.fillMode == FillWire) {
            gpu->pipeline       (cmd, pipeline);
            gpu->graphicsRoot   (cmd, root);
            gpu->graphicsCbv    (cmd, 2, passData);
            gpu->graphicsTable  (cmd, 3, 0);
            gpu->topology       (cmd, PrimitiveTopology::TriangleList);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                int meshId = model.meshId;
                int materialId = model.materialId;
                if (meshId < 0) {
                    continue;
                }
                MeshData&     m   = meshRegistry.get({meshId});
                MaterialData& mat = materialRegistry.get({materialId});
                MaterialData  mat2 = mat;
                mat2.baseColorMap    = gpu->getTextureViewBind({ mat.baseColorMap });
                mat2.ormMap          = gpu->getTextureViewBind({ mat.ormMap });
                mat2.normalMap       = gpu->getTextureViewBind({ mat.normalMap });
                mat2.emissiveMap     = gpu->getTextureViewBind({ mat.emissiveMap });

    
                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(transform.v));
                XMMATRIX mvpMat = modelMat * viewMat * projMat;

                ObjectData objectData = {};
                XMStoreFloat4x4(&objectData.m, modelMat);
                // XMStoreFloat4x4(&objectData.mvp, XMMatrixTranspose(mvp)));
                
                gpu->graphicsCbv  (cmd, 0, objectData);
                gpu->graphicsCbv  (cmd, 1, mat2);
                gpu->indexBuffer  (cmd, m.indices);
                gpu->vertexBuffer (cmd, 0, m.position);
                gpu->vertexBuffer (cmd, 1, m.normal);
                gpu->vertexBuffer (cmd, 2, m.uv);
                gpu->vertexBuffer (cmd, 3, m.tangent);
                gpu->drawIndexed  (cmd, m.vCount, 1, 0, 0, 0);
            }
        }


        gpu->barrier(cmd, target.texture, State::Present);
        gpu->end(cmd);
        gpu->execute(queue, cmd);
        gpu->present(swapchain, false);
        gpu->signal(queue, cmd);
        gpu->printErrors();
        endEvent();
        frameIdx++;
        return 1;
    }

    Buffer create(const char* name, uint32_t size, const uint8_t* data) {
        wait();
        Buffer buffer = gpu->createBuffer(name, BufferUpload, size);
        gpu->write(buffer, 0, size, data);
        return buffer;
    }
    
    Mesh create(const MeshData& desc) {
        return meshRegistry.create(desc);
    }

    MaterialTexture create(const char* name, const uint8_t* data, uint32_t size) {
        return materialRegistry.create(name, data, size);
    }
    
    Material create(const char* name, MaterialDesc& desc) {
        return materialRegistry.create(name, desc);
    }
    
    void destroy(Buffer buffer) {
        gpu->destroy(buffer);
    }
    
    void destroy(MaterialTexture materialTexture) {
        materialRegistry.destroy(materialTexture);
    }
    
    void destroy(Material material) {
        materialRegistry.destroy(material);
    }


private:
    bool                    vulkan;
    std::unique_ptr<IGpu>   gpu;
    Swapchain               swapchain;
    Queue                   queue;
    Buffer                  uploadBuffer;
    Command                 mainCommand;
    Root                    root;
    Pipeline                pipeline;
    Pipeline                pipelineWire;
    FrameControl            frameControl;
    MeshRegistry            meshRegistry;
    MaterialRegistry        materialRegistry;
    Texture                 depthTexture;
    TextureView             depthView;
    Shadow                  shadow;
    Viewport                viewport;
    Rect                    scissorRect;
};  

std::unique_ptr<IRenderer> createRendererPbr(bool vulkan) {
    return std::make_unique<Renderer>(vulkan);
}
}