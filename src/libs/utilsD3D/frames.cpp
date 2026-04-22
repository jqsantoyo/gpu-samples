#include "frames.h"
#include <utils/utils.h>
using namespace DirectX;
using namespace Microsoft::WRL;


namespace gpu {






bool FrameControl::init(Device& device, Queue* queue, int frameCount, uint32_t maxMemory) {
    this->queue = queue;
    this->maxMemory = align256(maxMemory);
    frames.resize(frameCount);
    for (auto& frame : frames) {
        GUARDHR(device.obj->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.cmdAllocator)));
        frame.fenceValue = 0;
        if (maxMemory != 0) {
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
            auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(this->maxMemory);
            HRESULT res = device.obj->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&frame.constantBuffer)
            );
            frame.constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&frame.data));
        } else {
            frame.data = nullptr;
        }
        
    }
    GUARDHR(device.obj->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames[0].cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));
    GUARDHR(cmdList.Get()->Close());
    return true;
}

bool FrameControl::begin(ID3D12PipelineState* initialState) {
    frameIdx = frameCounter % frames.size();
    Frame& frame = frames[frameIdx];
    queue->wait(frame.fenceValue);
    GUARDHR(frame.cmdAllocator->Reset());
    GUARDHR(cmdList->Reset(frame.cmdAllocator.Get(), initialState));
    frame.allocatedMemory = 0;
    return true;
}

bool FrameControl::execute() {
    Frame& frame = frames[frameIdx];
    HRESULT res = cmdList->Close();
    queue->execute({ cmdList.Get() });
    return true;
}

bool FrameControl::end() {
    Frame& frame = frames[frameIdx];
    GUARD(queue->signal(frame.fenceValue));
    frameCounter++;
    return true;
}

void FrameControl::barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
    cmdList->ResourceBarrier(1, &barrier);
}

void FrameControl::heaps(ID3D12DescriptorHeap* cbvSrvUavHeap, ID3D12DescriptorHeap* samplerHeap) {
    ID3D12DescriptorHeap* heaps[] = { cbvSrvUavHeap, samplerHeap };
    int heapCount = samplerHeap == nullptr ? 1 : 2;
    cmdList->SetDescriptorHeaps(heapCount, heaps);
}

void FrameControl::setConstantBuffer(UINT idx, void* data, int dataSize) {
    Frame& frame = frames[frameIdx];
    memcpy(frame.data + frame.allocatedMemory, data, dataSize);
    D3D12_GPU_VIRTUAL_ADDRESS addr = frame.constantBuffer->GetGPUVirtualAddress() + frame.allocatedMemory;
    cmdList->SetGraphicsRootConstantBufferView(idx, addr);
    frame.allocatedMemory += dataSize;
}





}