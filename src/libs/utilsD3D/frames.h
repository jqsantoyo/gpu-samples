#pragma once
#include "core.h"

namespace gpu {



struct Frame {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  cmdAllocator;
    UINT64                                          fenceValue;
    Microsoft::WRL::ComPtr<ID3D12Resource>          constantBuffer;
    uint32_t                                        allocatedMemory;
    uint8_t*                                        data;
};

class FrameControl {
public:
    bool init(Device& device, Queue* queue, int frameCount, uint32_t maxMemory = 0);
    bool begin(ID3D12PipelineState* initialState);
    bool execute();
    bool end();
    void barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
    void heaps(ID3D12DescriptorHeap* cbvSrvUavHeap, ID3D12DescriptorHeap* samplerHeap = nullptr);

    template<typename T>
    void setConstantBuffer(UINT idx, T& t) {
        uint32_t dataSize = align256(sizeof(T));
        setConstantBuffer(idx, &t, dataSize);
    }

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   cmdList;
private:
    Queue*                                              queue;
    std::vector<Frame>                                  frames;
    int                                                 frameCounter = 0;
    int                                                 frameIdx     = 0;
    uint32_t                                            maxMemory;
    void setConstantBuffer(UINT idx, void* data, int dataSize);
};




}