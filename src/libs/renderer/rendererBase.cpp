#include "rendererBase.h"

namespace gpu {



void RendererBase::init(const RendererBaseDesc& desc) {
    viewport = { 0.0f, 0.0f, static_cast<float>(desc.windowSize.x), static_cast<float>(desc.windowSize.y), 0, 1 };
    scissorRect = { 0, 0, desc.windowSize.x, desc.windowSize.y };
    uint32_t frameCount = 2;


    gpu          = createGpu();
    queue        = gpu->createQueue();
    swapchain    = gpu->createSwapchain(queue, desc.window, desc.windowSize.x, desc.windowSize.y, frameCount);
    mainCommand  = gpu->createCommand();
    uploadBuffer = gpu->createBuffer("UploadBuffer", BufferUpload, 2048 * 2048 * 4);
    
    cmds.resize(frameCount);
    for (auto& cmd : cmds) {
        cmd = gpu->createCommand(QueueGraphics, desc.frameMemory);
    }

    staticBuffers   .init(desc.staticBufferCount);
    meshes          .init(desc.meshCount);
    materials       .init(desc.materialCount);
    materialTextures.init(desc.materialTextureCount);
    
    // defaultBaseColorMap = createDefault({1, 1, 0, 1});   // default base color (magenta)
    defaultBaseColorMap = create("defaultColor", { 1, 1, 1, 1 });   // default base color
    defaultORMMap       = create("defaultOrm", { 1, 1, 0, 1 });   // default ORM
    defaultNormalMap    = create("defaultNormal", { 1, .5, .5, 1 }); // default normals
    defaultEmissiveMap  = create("defaultEmissive", { 1, 0, 0, 0 });   // default emissive
    init2();
    reset();
}

void RendererBase::terminate() {
    gpu->wait(queue);
    terminate2();
}

void RendererBase::reset() {
    wait();
    meshes.reset();
    reset2();
}


void RendererBase::resize(int width, int height) {
    resize2(width, height);
}

void RendererBase::render(const RenderView& view) {
    static uint64_t frameIdx = 0;
    beginEvent("Render %llu", frameIdx);
    SwapTarget target = gpu->next(swapchain);
    frameIdx++;
    frameIdx %= cmds.size();
    Command cmd = cmds[frameIdx];
    render2(target, cmd, view);
    gpu->printErrors();
    endEvent();
    frameIdx++;
}

void RendererBase::wait() {
    gpu->wait(queue);
}

StaticBuffer RendererBase::create(const char* name, uint32_t size, const uint8_t* data) {
    wait();
    StaticBuffer staticBuffer = staticBuffers.alloc();
    StaticBufferData& staticBufferData = staticBuffers[staticBuffer];
    staticBufferData.buffer = gpu->createBuffer(name, BufferUpload, size);
    gpu->write(staticBufferData.buffer, 0, size, data);
    return staticBuffer;
}

Mesh RendererBase::create(const MeshDesc& desc) {
    Mesh mesh = meshes.alloc();
    MeshData& meshData = meshes[mesh];
    StaticBufferData& staticBufferData = staticBuffers[desc.staticBuffer];
    Buffer buffer = staticBufferData.buffer;
    meshData = {
        .vCount         = desc.vCount,
        .indices        = { buffer, desc.offsetIndices,  desc.sizeIndices,    desc.formatIndices },
        .position       = { buffer, desc.offsetPosition, desc.sizePosition,   sizeof(float) * 3  },
        .normal         = { buffer, desc.offsetNormal,   desc.sizeNormal,     sizeof(float) * 3  },
        .uv             = { buffer, desc.offsetUv,       desc.sizeUv,         sizeof(float) * 2  },
        .tangent        = { buffer, desc.offsetTangent,  desc.sizeTangent,    sizeof(float) * 4  },
        .color          = { buffer, desc.offsetColor,    desc.sizeColor,      sizeof(float) * 3  },
    };
    return mesh;
}

MaterialTexture RendererBase::create(const char* name, vec4 color) {
    MaterialTexture      materialTexture     = materialTextures.alloc();
    MaterialTextureData& materialTextureData = materialTextures[materialTexture];
    materialTextureData.texture     = gpu->createTexture2(name, TextureUsage::Default, Format::RGBA8un, { 1, 1, 1});
    materialTextureData.textureView = gpu->createTextureView(materialTextureData.texture);
    uint32_t pixel = toUint(color);
    std::vector<ResourceData> subresources = { { reinterpret_cast<void*>(&pixel), 0, 0, }, };
    upload(materialTextureData.texture, subresources);
    return materialTexture;
}

MaterialTexture RendererBase::create(const char* name, const uint8_t* data, uint32_t size) {
    MaterialTexture      materialTexture     = materialTextures.alloc();
    MaterialTextureData& materialTextureData = materialTextures[materialTexture];
    std::vector<ResourceData> subresources;
    materialTextureData.texture     = gpu->createTexture(name, data, size, subresources);
    materialTextureData.textureView = gpu->createTextureView(materialTextureData.texture);
    upload(materialTextureData.texture, subresources);
    // printf("Texture[%4d]: name: %-40s resource: %3d srv: %3d size: %llu\n", id, name, texture.id.idx, texture.srv.idx, uploadBufferSize); 
    return materialTexture;
}

Material RendererBase::create(const char* name, MaterialDesc& desc) {
    Material material = materials.alloc(name);
    MaterialData& materialData = materials[material];
    MaterialTexture baseColorMap  = desc.baseColorMap.idx != -1 ? desc.baseColorMap : defaultBaseColorMap;
    MaterialTexture ormMap        = desc.ormMap.idx       != -1 ? desc.ormMap       : defaultORMMap;
    MaterialTexture normalMap     = desc.normalMap.idx    != -1 ? desc.normalMap    : defaultNormalMap;
    MaterialTexture emissiveMap   = desc.emissiveMap.idx  != -1 ? desc.emissiveMap  : defaultEmissiveMap;
    materialData = {
        .baseColor      = desc.baseColor,
        .emissive       = desc.emissive,
        .metallic       = desc.metallic,
        .roughness      = desc.roughness,
        .baseColorMap   = materialTextures[baseColorMap].textureView.idx,
        .ormMap         = materialTextures[ormMap].textureView.idx,
        .normalMap      = materialTextures[normalMap].textureView.idx,
        .emissiveMap    = materialTextures[emissiveMap].textureView.idx,
    };
    return material;
}

void RendererBase::destroy(StaticBuffer staticBuffer) {
    StaticBufferData& staticBufferData = staticBuffers[staticBuffer];
    gpu->destroy(staticBufferData.buffer);
    staticBuffers.free(staticBuffer);
}

void RendererBase::destroy(MaterialTexture materialTexture) {
    MaterialTextureData& materialTextureData = materialTextures[materialTexture];
    gpu->destroy(materialTextureData.texture);
    gpu->destroy(materialTextureData.textureView);
    materialTextures.free(materialTexture);
}

void RendererBase::destroy(Material material) {
    materials.free(material);
}


bool RendererBase::upload(Texture texture, const std::vector<ResourceData>& subresources) {
    // const UINT64 uploadBufferSize = GetRequiredIntermediateSize(res, 0, subresources.size());
    gpu->wait        (queue);
    gpu->begin       (mainCommand);
    gpu->barrier     (mainCommand, texture, State::CopyDest);
    gpu->subresources(mainCommand, texture, uploadBuffer, 0, 0, subresources);
    gpu->barrier     (mainCommand, texture, State::PSRead);
    gpu->end         (mainCommand);
    gpu->execute     (queue, mainCommand);
    return true;
}


}
