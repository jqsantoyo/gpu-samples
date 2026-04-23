#include "resources.h"
#include <utils/utils.h>
#include <ddsTextureLoader/DDSTextureLoader12.h>
using namespace DirectX;
using namespace Microsoft::WRL;


namespace gpu {


bool MeshRegistry::init() {
    return true;
}

void MeshRegistry::reset(int buffersIdx, int meshesIdx) {
    buffers.resize(buffersIdx);
    meshes.resize(meshesIdx);
}

int MeshRegistry::addBuffer(Device& device, const BufferDesc& desc) {
    buffers.push_back(Buffer());
    Buffer& b = buffers.back();

    HRESULT res;
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.size);
    res = device.obj->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&b.vbUp)
    );
    if (FAILED(res)) {
        return false;
    }

    UINT8* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
    GUARDHR(b.vbUp->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
    memcpy(vertexDataBegin, desc.data + desc.offset, desc.size);
    b.vbUp->Unmap(0, nullptr);
    return buffers.size() - 1;
}

int MeshRegistry::addMesh(const MeshDesc& desc) {
    Buffer& buffer = buffers[desc.bufferId];
    meshes.push_back(Mesh());
    Mesh& m = meshes.back();
    m = {
        .bufferId = desc.bufferId,
        .vCount = desc.vCount,
        .indicesView = {
            .BufferLocation = buffer.vbUp->GetGPUVirtualAddress() + desc.indices.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.indices.size),
            .Format = DXGI_FORMAT_R16_UINT,
        },
        .positionView = {
            .BufferLocation = buffer.vbUp->GetGPUVirtualAddress() + desc.position.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.position.size),
            .StrideInBytes = sizeof(float) * 3,
        },
        .normalView = {
            .BufferLocation = buffer.vbUp->GetGPUVirtualAddress() + desc.normal.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.normal.size),
            .StrideInBytes = sizeof(float) * 3,
        },
        .uvView = {
            .BufferLocation = buffer.vbUp->GetGPUVirtualAddress() + desc.uv.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.uv.size),
            .StrideInBytes = sizeof(float) * 2,
        },
        .tangentView = {
            .BufferLocation = buffer.vbUp->GetGPUVirtualAddress() + desc.tangent.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.tangent.size),
            .StrideInBytes = sizeof(float) * 4,
        },
        .colorView = {
            .BufferLocation = buffer.vbUp->GetGPUVirtualAddress() + desc.color.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.color.size),
            .StrideInBytes = sizeof(float) * 3,
        },
    };
    return meshes.size() - 1;
}

Mesh& MeshRegistry::getMesh(int idx) {
    return meshes[idx];
}


int MeshRegistry::getBufferCount() {
    return buffers.size();
}

int MeshRegistry::getMeshCount() {
    return meshes.size();
}











bool TextureRegistry::init(Device* device, Queue* queue) {
    this->device = device;
    this->queue = queue;
    GUARDHR(device->obj->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
    GUARDHR(device->obj->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));
    GUARDHR(cmdList->Close());
    return true;
}

void TextureRegistry::reset(int idx) {
    textures.resize(idx);
}

int TextureRegistry::addTexture(const char* filename) {
    std::vector<uint8_t> data;
    GUARD(readFile(filename, data));
    return addTexture(data.data(), data.size());
}

int TextureRegistry::addTexture(const uint8_t* data, uint32_t size) {
    textures.push_back({});
    Texture& texture = textures.back();

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    GUARDHR(LoadDDSTextureFromMemory(device->obj.Get(), data, size, texture.res.GetAddressOf(), subresources));

    GUARD(device->cbvHeap.next(texture.descriptor));
    D3D12_CPU_DESCRIPTOR_HANDLE texDesc = device->cbvHeap.getCpu(texture.descriptor);

    // Currently the sRGB format seems to have no effect on samping in shaders.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
        .Format = texture.res->GetDesc().Format,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = texture.res->GetDesc().MipLevels,
            .PlaneSlice = 0,
            .ResourceMinLODClamp = 0.0f,
        }
    };
    device->obj->CreateShaderResourceView(texture.res.Get(), &srvDesc, texDesc);

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.res.Get(), 0, subresources.size());
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    GUARDHR(device->obj->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&texture.resUp)));

    queue->wait();
    GUARDHR(cmdAllocator->Reset());
    GUARDHR(cmdList->Reset(cmdAllocator.Get(), nullptr));
    auto barr0 = CD3DX12_RESOURCE_BARRIER::Transition(texture.res.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    auto barr1 = CD3DX12_RESOURCE_BARRIER::Transition(texture.res.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barr0);
    UpdateSubresources(cmdList.Get(), texture.res.Get(), texture.resUp.Get(), 0, 0, subresources.size(), subresources.data());
    cmdList->ResourceBarrier(1, &barr1);
    HRESULT res2 = cmdList->Close();
    queue->execute({ cmdList.Get() });
    return textures.size() - 1;
}

int TextureRegistry::addDefault(vec4 color) {
    textures.push_back({});
    Texture& texture = textures.back();
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_HEAP_PROPERTIES upHeapProps(D3D12_HEAP_TYPE_UPLOAD);

    
    D3D12_RESOURCE_DESC desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = 1,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0,
        },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };
    GUARDHR(device->obj->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&texture.res)));
    texture.res->SetName(L"Default texture");
    
    uint32_t pixel =
        + (static_cast<uint32_t>(255.0f * color.x) << 24)
        + (static_cast<uint32_t>(255.0f * color.y) << 16)
        + (static_cast<uint32_t>(255.0f * color.z) <<  8)
        +  static_cast<uint32_t>(255.0f * color.w);

    std::vector<D3D12_SUBRESOURCE_DATA> subresources = {
        { .pData = reinterpret_cast<void*>(&pixel), .RowPitch = 0, .SlicePitch = 0, },
    };

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.res.Get(), 0, subresources.size());
    auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    GUARDHR(device->obj->CreateCommittedResource(
        &upHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &upDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&texture.resUp)));

    queue->wait();
    GUARDHR(cmdAllocator->Reset());
    GUARDHR(cmdList->Reset(cmdAllocator.Get(), nullptr));
    auto barr0 = CD3DX12_RESOURCE_BARRIER::Transition(texture.res.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    auto barr1 = CD3DX12_RESOURCE_BARRIER::Transition(texture.res.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barr0);
    UpdateSubresources(cmdList.Get(), texture.res.Get(), texture.resUp.Get(), 0, 0, subresources.size(), subresources.data());
    cmdList->ResourceBarrier(1, &barr1);
    HRESULT res2 = cmdList->Close();
    queue->execute({ cmdList.Get() });

    
    GUARD(device->cbvHeap.next(texture.descriptor));
    D3D12_CPU_DESCRIPTOR_HANDLE texDesc = device->cbvHeap.getCpu(texture.descriptor);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
        .Format = texture.res->GetDesc().Format,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = texture.res->GetDesc().MipLevels,
            .PlaneSlice = 0,
            .ResourceMinLODClamp = 0.0f,
        }
    };
    device->obj->CreateShaderResourceView(texture.res.Get(), &srvDesc, texDesc);

    return textures.size() - 1;
}

int TextureRegistry::getCount() {
    return textures.size();
}

Texture& TextureRegistry::get(int idx) {
    return textures[idx];
}












bool MaterialRegistry::init(Device* device) {
    this->device = device;
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