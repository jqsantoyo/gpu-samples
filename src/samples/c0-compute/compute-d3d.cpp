#include "compute.h"
#include <gpuD3D/gpu.h>
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace gpu::gpu;

namespace gpu {

class Compute : public ICompute {
public:
    bool init() {
        gpu.init(0, 0, 0);
        queue    = gpu.createQueue();
        cmd      = gpu.createCommand();
        root     = gpu.createRoot({ rootSrv(0), rootSrv(1), rootUav(2) }, {});
        pipeline = gpu.createPipeline(root, "shader");
        return true;
    }

    bool compute(int count, vec3* a, vec3* b, vec3* c) {
        uint32_t buffSize = count * sizeof(vec3);
        
        Buffer buffA = gpu.createBuffer(buffSize, D3D12_HEAP_TYPE_UPLOAD,   D3D12_RESOURCE_STATE_GENERIC_READ);
        Buffer buffB = gpu.createBuffer(buffSize, D3D12_HEAP_TYPE_UPLOAD,   D3D12_RESOURCE_STATE_GENERIC_READ);
        Buffer buffX = gpu.createBuffer(buffSize, D3D12_HEAP_TYPE_DEFAULT,  D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        Buffer buffC = gpu.createBuffer(buffSize, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COMMON);
        gpu.write(buffA, 0, buffSize, reinterpret_cast<uint8_t*>(a));
        gpu.write(buffB, 0, buffSize, reinterpret_cast<uint8_t*>(b));



        int threads = 64;
        int groupsX = (count + threads - 1) / threads;

        // queue.wait();
        gpu.begin(cmd, pipeline);
        gpu.computeRoot(cmd, root);
        gpu.computeSrv(cmd, 0, buffA);
        gpu.computeSrv(cmd, 1, buffB);
        gpu.computeUav(cmd, 2, buffX);
        gpu.dispatch(cmd, groupsX, 1, 1);
        gpu.copy(cmd, buffC, buffX);
        gpu.end(cmd);
        gpu.execute(queue, cmd);
        gpu.wait(queue);

        gpu.read(buffC, [&](uint8_t* data) { memcpy(c, data, buffSize); });
        return true;
    }

    void terminate() {
    }

private:
    Gpu         gpu;
    Queue       queue;
    Command     cmd;
    Root        root;
    Pipeline    pipeline;
};

std::unique_ptr<ICompute> createComputeD3D() {
    return std::make_unique<Compute>();
}

}