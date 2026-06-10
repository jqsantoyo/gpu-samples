#include "renderUtils.h"
#include <utils/utils.h>
#include <d3dx12.h>
using namespace DirectX;
using namespace Microsoft::WRL;


namespace gpu {








bool DepthBuffer::init(gpu::Gpu* gpu, uint32_t width, uint32_t height) {
    gpu::TextureDesc desc = {
        .heap       = D3D12_HEAP_TYPE_DEFAULT,
        .state      = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        .width      = width,
        .height     = height,
        .format     = DXGI_FORMAT_D32_FLOAT,
        .layout     = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .flags      = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        .hasClearValue = true,
        .clearValue = {
                        .Format = DXGI_FORMAT_D32_FLOAT,
                        .DepthStencil = { .Depth = 1, .Stencil = 0 },
                      },
    };
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {
        .Format         = DXGI_FORMAT_D32_FLOAT,
        .ViewDimension  = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags          = D3D12_DSV_FLAG_NONE,
        .Texture2D      = { .MipSlice = 0 },
    };
    texture = gpu->createTexture(desc);
    dsv = gpu->nextDsv(0, texture, dsvDesc);
    return true;
}


bool Shadow::init(gpu::Gpu* gpu, gpu::Root root, uint32_t width, uint32_t height) {
    this->width = width;
    this->height = height;

    gpu::PsoGraphicsDesc pipelineDesc = {
        .vs                 = "shadow_v",
        .ps                 = "shadow_p",
        .blendState         = gpu::defaultBlend(),
        .sampleMask         = UINT_MAX,
        .rasterizerState    = gpu::defaultFill(),
        .depthStencilState  = gpu::defaultDepth(),
        .inputLayout        = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        },
        .topology           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .numRenderTargets   = 1,
        .rtvFormats         = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .dsvFormat          = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .sampleDesc         = { .Count = 1, .Quality = 0 },
    };

    gpu::TextureDesc textureDesc = {
        .heap       = D3D12_HEAP_TYPE_DEFAULT,
        .state      = D3D12_RESOURCE_STATE_GENERIC_READ,
        .width      = width,
        .height     = height,
        .format     = DXGI_FORMAT_R24G8_TYPELESS,
        .layout     = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .flags      = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        .hasClearValue = true,
        .clearValue = {
            .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
            .DepthStencil = { .Depth = 1, .Stencil = 0 },
        },
    };
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
        .Format                     = DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
        .ViewDimension              = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = 1,
            .PlaneSlice = 0,
            .ResourceMinLODClamp = 0.0f,
        }
    };
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {
        .Format         = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .ViewDimension  = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags          = D3D12_DSV_FLAG_NONE,
        .Texture2D      = { .MipSlice = 0 },
    };
    pipeline = gpu->createPipeline(root, pipelineDesc);
    target   = gpu->createTexture(textureDesc);
    srv = gpu->nextSrv(0, target, srvDesc);
    dsv = gpu->nextDsv(0, target, dsvDesc);
    return true;
}
















bool FrameControl::init(gpu::Gpu& gpu, int frameCount, uint32_t maxMemory) {
    cmds.resize(frameCount);
    for (auto& cmd : cmds) {
        cmd = gpu.createCommand(D3D12_COMMAND_LIST_TYPE_DIRECT, maxMemory);
    }
    return true;
}

gpu::Command FrameControl::next() {
    frameIdx++;
    frameIdx %= cmds.size();
    return cmds[frameIdx];
}




bool MeshRegistry::init(gpu::Gpu* gpu) {
    this->gpu = gpu;
    return true;
}

void MeshRegistry::reset(int meshesIdx) {
    meshes.resize(meshesIdx);
}

int MeshRegistry::addMesh(const MeshDesc& desc) {
    meshes.push_back(Mesh());
    D3D12_GPU_VIRTUAL_ADDRESS addr = gpu->addr(gpu::Buffer{ desc.bufferId });
    Mesh& m = meshes.back();
    m = {
        .bufferId = desc.bufferId,
        .vCount = desc.vCount,
        .indicesView = {
            .BufferLocation = addr + desc.indices.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.indices.size),
            .Format = DXGI_FORMAT_R16_UINT,
        },
        .positionView = {
            .BufferLocation = addr + desc.position.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.position.size),
            .StrideInBytes = sizeof(float) * 3,
        },
        .normalView = {
            .BufferLocation = addr + desc.normal.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.normal.size),
            .StrideInBytes = sizeof(float) * 3,
        },
        .uvView = {
            .BufferLocation = addr + desc.uv.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.uv.size),
            .StrideInBytes = sizeof(float) * 2,
        },
        .tangentView = {
            .BufferLocation = addr + desc.tangent.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.tangent.size),
            .StrideInBytes = sizeof(float) * 4,
        },
        .colorView = {
            .BufferLocation = addr + desc.color.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.color.size),
            .StrideInBytes = sizeof(float) * 3,
        },
    };
    return meshes.size() - 1;
}

Mesh& MeshRegistry::getMesh(int idx) {
    return meshes[idx];
}

int MeshRegistry::getMeshCount() {
    return meshes.size();
}







bool TextureRegistry::init(gpu::Gpu* gpu, gpu::Queue queue, gpu::Command cmd, gpu::Buffer uploadBuffer, int page) {
    this->gpu        = gpu;
    this->queue         = queue;
    this->cmd           = cmd;
    this->uploadBuffer  = uploadBuffer;
    this->page          = page;
    return true;
}

int TextureRegistry::getCount() {
    return textures.size();
}

int TextureRegistry::getSrv(int idx) {
    return textures[idx].srv.idx;
}
    // const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureData.res.Get(), 0, subresources.size());
    // auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    // GUARDHR(obj->CreateCommittedResource(
    //     &upHeapProps,
    //     D3D12_HEAP_FLAG_NONE,
    //     &upDesc,
    //     D3D12_RESOURCE_STATE_GENERIC_READ,
    //     nullptr,
    //     IID_PPV_ARGS(&textureData.resUp)));
int TextureRegistry::add(vec4 color) {
    int id = textures.size();
    textures.push_back({});
    Texture& texture = textures.back();


    gpu::TextureDesc texDesc = {
        .heap       = D3D12_HEAP_TYPE_DEFAULT,
        .state      = D3D12_RESOURCE_STATE_COMMON,
        .width      = 1,
        .height     = 1,
        .format     = DXGI_FORMAT_R8G8B8A8_UNORM,
        .layout     = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .flags      = D3D12_RESOURCE_FLAG_NONE,
    };
    texture.id = gpu->createTexture(texDesc);
    ID3D12Resource* res = gpu->get(texture.id);
    
    uint32_t pixel = toUint(color);
    std::vector<D3D12_SUBRESOURCE_DATA> subresources = {
        { .pData = reinterpret_cast<void*>(&pixel), .RowPitch = 0, .SlicePitch = 0, },
    };
    upload(texture.id, subresources);
    createSrv(texture, res);
    return id;
}

int TextureRegistry::add(const char* filename) {
    std::vector<uint8_t> data;
    GUARD(readFile(filename, data));
    return add("file", data.data(), data.size());
}

int TextureRegistry::add(const char* name, const uint8_t* data, uint32_t size) {
    int id = textures.size();
    textures.push_back({});
    Texture& texture = textures.back();
    
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    texture.id = gpu->createTexture(data, size, subresources);
    ID3D12Resource* res = gpu->get(texture.id);
    
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(res, 0, subresources.size());
    upload(texture.id, subresources);
    
    createSrv(texture, res);
    printf("Texture[%4d]: name: %-40s resource: %3d srv: %3d size: %llu\n", id, name, texture.id.idx, texture.srv.idx, uploadBufferSize); 
    return id;
}

bool TextureRegistry::upload(gpu::Texture texture, std::vector<D3D12_SUBRESOURCE_DATA>& subresources) {
    gpu->wait        (queue);
    gpu->begin       (cmd);
    gpu->barrier     (cmd, texture, D3D12_RESOURCE_STATE_COPY_DEST);
    gpu->subresources(cmd, texture, uploadBuffer, 0, 0, subresources);
    gpu->barrier     (cmd, texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    gpu->end         (cmd);
    gpu->execute     (queue, cmd);
    return true;
}

bool TextureRegistry::createSrv(Texture& texture, ID3D12Resource* res) {
    D3D12_RESOURCE_DESC resDesc = res->GetDesc();
    // Currently the sRGB format seems to have no effect on sampling in shaders.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
        .Format = resDesc.Format,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = resDesc.MipLevels,
            .PlaneSlice = 0,
            .ResourceMinLODClamp = 0.0f,
        }
    };
    texture.srv = gpu->nextSrv(page, texture.id, srvDesc);
    return true;
}






bool MaterialRegistry::init(gpu::Gpu* gpu) {
    this->gpu = gpu;
    return true;
}

void MaterialRegistry::reset(int idx) {
    materials.resize(idx);
}

int MaterialRegistry::addMaterial(Material& material) {
    materials.push_back(material);
    return materials.size() - 1;
}

Material& MaterialRegistry::getMaterial(int idx) {
    return materials[idx];
}

int MaterialRegistry::getCount() {
    return materials.size();
}
























}