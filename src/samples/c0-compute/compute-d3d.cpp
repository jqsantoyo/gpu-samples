#include "compute.h"
#include <utilsD3D/utilsD3D.h>
using namespace DirectX;
using namespace Microsoft::WRL;

namespace gpu {

class Compute : public ICompute {
public:
    bool init() {
        GUARD(factory.init());
        GUARD(factory.select());
        GUARD(device.init(factory.getSelected(), 0, 0, 0));
        GUARD(queue.init(device, D3D12_COMMAND_LIST_TYPE_DIRECT));

        
        ComPtr<ID3DBlob>            sig;
        ComPtr<ID3DBlob>            error;
        CD3DX12_ROOT_PARAMETER rootParam[3];
        rootParam[0].InitAsShaderResourceView(0);
        rootParam[1].InitAsShaderResourceView(1);
        rootParam[2].InitAsUnorderedAccessView(2);
        
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(3, rootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
        GUARDHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
        GUARDHR(device.obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&root)));

        Shader shader;
        GUARD(shader.load("shader"));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
            .pRootSignature = root.Get(),
            .CS = shader.bytecode,
            .NodeMask = 0,
            .CachedPSO = { nullptr, 0 },
            .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
        };
        GUARDHR(device.obj->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
        
        fenceValue = 0;
        GUARDHR(device.obj->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
        GUARDHR(device.obj->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));
        GUARDHR(cmdList.Get()->Close());



        return true;
    }

    bool compute(int count, vec3* a, vec3* b, vec3* c) {
        uint32_t buffSize = count * sizeof(vec3);
        WriteBuffer buffA;
        WriteBuffer buffB;
        UAVBuffer   buffX;
        ReadBuffer  buffC;
        GUARD(buffA.init(device.obj.Get(), buffSize));
        GUARD(buffB.init(device.obj.Get(), buffSize));
        GUARD(buffX.init(device.obj.Get(), buffSize));
        GUARD(buffC.init(device.obj.Get(), buffSize));
        GUARD(buffA.write(0, buffSize, reinterpret_cast<uint8_t*>(a)));
        GUARD(buffB.write(0, buffSize, reinterpret_cast<uint8_t*>(b)));

        auto barr0 = CD3DX12_RESOURCE_BARRIER::Transition(buffX.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
        auto barr1 = CD3DX12_RESOURCE_BARRIER::Transition(buffX.get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
        int threads = 64;
        int groupsX = (count + threads - 1) / threads;

        // queue.wait();
        GUARDHR(cmdAllocator->Reset());
        GUARDHR(cmdList->Reset(cmdAllocator.Get(), pso.Get()));
        cmdList->SetComputeRootSignature(root.Get());
        cmdList->SetComputeRootShaderResourceView (0, buffA.get()->GetGPUVirtualAddress());
        cmdList->SetComputeRootShaderResourceView (1, buffB.get()->GetGPUVirtualAddress());
        cmdList->SetComputeRootUnorderedAccessView(2, buffX.get()->GetGPUVirtualAddress());
        cmdList->Dispatch(groupsX, 1, 1);
        cmdList->ResourceBarrier(1, &barr0);
        cmdList->CopyResource(buffC.get(), buffX.get());
        cmdList->ResourceBarrier(1, &barr1);
        GUARDHR(cmdList->Close());
        queue.execute({ cmdList.Get() });
        queue.wait();

        vec3* dataC = reinterpret_cast<vec3*>(buffC.start());
        if (dataC) {
            memcpy(c, dataC, buffSize);
        }
        buffC.stop();
        return true;
    }
    void terminate() {
    }

private:
    Factory factory;
    Device device;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         root;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         pso;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      cmdAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   cmdList;
    Queue                                               queue;
    UINT64                                              fenceValue;
};

std::unique_ptr<ICompute> createComputeD3D() {
    return std::make_unique<Compute>();
}

}