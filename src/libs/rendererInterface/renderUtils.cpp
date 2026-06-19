#include "renderUtils.h"
#include <utils/utils.h>
#include <directxmath.h>


namespace gpu {








bool Shadow::init(IGpu* gpu, Root root, uint32_t width, uint32_t height) {
    this->width = width;
    this->height = height;

    PsoGraphicsDesc pipelineDesc = {
        .vs                 = "shadow_v",
        .ps                 = "shadow_p",
        .fillMode           = FillMode::Solid,
        .cullMode           = CullMode::None,
        .enableDepth        = true,
        .topologyType       = PrimitiveTopologyType::Triangle,
        .inputElements      = {
            { "POSITION", 0, Format::RGB32f,  0, 0 },
        },
        .renderTargets      = { { Format::RGBA8un, true } },
        .dsvFormat          = Format::D24S8,
        .samples            = 1,
    };
    pipeline  = gpu->createPipeline(root, pipelineDesc);
    target    = gpu->createTexture2("ShadowTex", TextureUsage::DepthStencil, Format::D24S8, { width, height, 1 }, 1, 1, ClearValue(1.0f, 0u));
    depthView = gpu->createDepthView(target);
    readView  = gpu->createTextureView(target);
    return true;
}
















bool FrameControl::init(IGpu* gpu, int frameCount, uint32_t maxMemory) {
    cmds.resize(frameCount);
    for (auto& cmd : cmds) {
        cmd = gpu->createCommand(QueueGraphics, maxMemory);
    }
    return true;
}

Command FrameControl::next() {
    frameIdx++;
    frameIdx %= cmds.size();
    return cmds[frameIdx];
}




bool MeshRegistry::init(IGpu* gpu, int meshCount) {
    this->gpu = gpu;
    meshes.init(meshCount);
    return true;
}

void MeshRegistry::reset() {
    meshes.reset();
}

Mesh MeshRegistry::create(const MeshData& desc) {
    Mesh mesh = meshes.alloc();
    MeshData& meshData = meshes[mesh];
    meshData = desc;
    return mesh;
}

MeshData& MeshRegistry::get(Mesh mesh) {
    return meshes[mesh];
}









bool MaterialRegistry::init(IGpu* gpu, Queue queue, Command cmd, Buffer uploadBuffer, int materialTextureCount, int materialCount) {
    this->gpu           = gpu;
    this->queue         = queue;
    this->cmd           = cmd;
    this->uploadBuffer  = uploadBuffer;
    materials.init(materialCount);
    materialTextures.init(materialTextureCount);
    
    // defaultBaseColorMap = createDefault({1, 1, 0, 1});   // default base color (magenta)
    defaultBaseColorMap = create("defaultColor", { 1, 1, 1, 1 });   // default base color
    defaultORMMap       = create("defaultOrm", { 1, 1, 0, 1 });   // default ORM
    defaultNormalMap    = create("defaultNormal", { 1, .5, .5, 1 }); // default normals
    defaultEmissiveMap  = create("defaultEmissive", { 1, 0, 0, 0 });   // default emissive
    return true;
}

MaterialTexture MaterialRegistry::create(const char* name, vec4 color) {
    MaterialTexture      materialTexture     = materialTextures.alloc();
    MaterialTextureData& materialTextureData = materialTextures[materialTexture];
    materialTextureData.texture     = gpu->createTexture2(name, TextureUsage::Default, Format::RGBA8un, { 1, 1, 1});
    materialTextureData.textureView = gpu->createTextureView(materialTextureData.texture);
    uint32_t pixel = toUint(color);
    std::vector<ResourceData> subresources = { { reinterpret_cast<void*>(&pixel), 0, 0, }, };
    upload(materialTextureData.texture, subresources);
    return materialTexture;
}

MaterialTexture MaterialRegistry::create(const char* name, const uint8_t* data, uint32_t size) {
    MaterialTexture      materialTexture     = materialTextures.alloc();
    MaterialTextureData& materialTextureData = materialTextures[materialTexture];
    std::vector<ResourceData> subresources;
    materialTextureData.texture     = gpu->createTexture(name, data, size, subresources);
    materialTextureData.textureView = gpu->createTextureView(materialTextureData.texture);
    upload(materialTextureData.texture, subresources);
    // printf("Texture[%4d]: name: %-40s resource: %3d srv: %3d size: %llu\n", id, name, texture.id.idx, texture.srv.idx, uploadBufferSize); 
    return materialTexture;
}


Material MaterialRegistry::create(const char* name, MaterialDesc& desc) {
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

void MaterialRegistry::destroy(MaterialTexture materialTexture) {
    MaterialTextureData& materialTextureData = materialTextures[materialTexture];
    gpu->destroy(materialTextureData.texture);
    gpu->destroy(materialTextureData.textureView);
    materialTextures.free(materialTexture);
}

void MaterialRegistry::destroy(Material material) {
    materials.free(material);
}

MaterialData& MaterialRegistry::get(Material material) {
    return materials[material];
}

bool MaterialRegistry::upload(Texture texture, const std::vector<ResourceData>& subresources) {
    // const UINT64 uploadBufferSize = GetRequiredIntermediateSize(res, 0, subresources.size());
    gpu->wait        (queue);
    gpu->begin       (cmd);
    gpu->barrier     (cmd, texture, State::CopyDest);
    gpu->subresources(cmd, texture, uploadBuffer, 0, 0, subresources);
    gpu->barrier     (cmd, texture, State::PSRead);
    gpu->end         (cmd);
    gpu->execute     (queue, cmd);
    return true;
}



























}