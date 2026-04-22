

#pragma once
#include "core.h"

namespace gpu {



class RootSig {
public:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> obj;
    bool initVoid(Device& device);
    bool init1Cbv(Device& device);
    bool init1Cbv1TableNSamplers(Device& device);
    bool init3Cbv1TableNSamplers(Device& device);

};



class PipelineBasic {
public:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> obj;
    bool init(Device& device, RootSig& sig);
};

class PipelineFill {
public:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> obj;
    bool init(Device& device, RootSig& sig);
};

class PipelineWire {
public:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> obj;
    bool init(Device& device, RootSig& sig);
};

class PipelineTex {
public:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> obj;
    bool init(Device& device, RootSig& sig);
};

class PipelineLights {
public:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> obj;
    bool init(Device& device, RootSig& sig);
};


class DepthBuffer {
public:
    bool init(Device* device, uint64_t width, uint32_t height);
    void reset();
    int dsvIdx;
private:
    Device* device;
    Microsoft::WRL::ComPtr<ID3D12Resource> obj;
};


class Shadow {
public:
    bool init(Device* device, ID3D12RootSignature* root, uint32_t width, uint32_t height);
    bool reset();
    bool render(const RenderView& view);
    // int getSrv();
    // int getDsv();
// private:
    Device* device;
    int srvIdx;
    int dsvIdx;
    Microsoft::WRL::ComPtr<ID3D12Resource> target;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    uint32_t width;
    uint32_t height;
};
}