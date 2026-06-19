#include "compute.h"
#include <gpu/gpu.h>

namespace gpu {

class Compute : public ICompute {
public:
    bool init() {
        GpuDesc gpuDesc = {
            .queueCount         = 1,
            .commandCount       = 1,
            .rootCount          = 1,
            .pipelineCount      = 1,
            .swapchainCount     = 0,
            .bufferCount        = 4,
            .textureCount       = 0,
            .maxColorViews      = 0,
            .maxRenderViews     = 0,
            .maxDepthViews      = 0,
        };
        gpu = createGpu(gpuDesc);
        queue    = gpu->createQueue();
        cmd      = gpu->createCommand();
        root     = gpu->createRoot({
            RootParam::binding(RootBinding::Read,      0),
            RootParam::binding(RootBinding::Read,      1),
            RootParam::binding(RootBinding::ReadWrite, 2)
        }, {});
        pipeline = gpu->createPipeline(root, "shader");
        return true;
    }

    bool compute(int count, vec3* a, vec3* b, vec3* c) {
        uint32_t buffSize = count * sizeof(vec3);
        
        Buffer buffA = gpu->createBuffer("buffA", BufferUpload, buffSize);
        Buffer buffB = gpu->createBuffer("buffB", BufferUpload, buffSize);
        Buffer buffX = gpu->createBuffer("buffX", BufferWrite,  buffSize);
        Buffer buffC = gpu->createBuffer("buffC", BufferRead,   buffSize);
        gpu->write(buffA, 0, buffSize, reinterpret_cast<uint8_t*>(a));
        gpu->write(buffB, 0, buffSize, reinterpret_cast<uint8_t*>(b));



        int threads = 64;
        int groupsX = (count + threads - 1) / threads;

        // queue.wait();
        gpu->begin(cmd, pipeline);
        gpu->computeRoot(cmd, root);
        gpu->computeSrv(cmd, 0, buffA);
        gpu->computeSrv(cmd, 1, buffB);
        gpu->computeUav(cmd, 2, buffX);
        gpu->dispatch(cmd, groupsX, 1, 1);
        gpu->copy(cmd, buffC, buffX);
        gpu->end(cmd);
        gpu->execute(queue, cmd);
        gpu->wait(queue);

        gpu->read(buffC, [&](uint8_t* data) { memcpy(c, data, buffSize); });
        return true;
    }

    void terminate() {
    }

private:
    std::unique_ptr<IGpu>   gpu;
    Queue                   queue;
    Command                 cmd;
    Root                    root;
    Pipeline                pipeline;
};

std::unique_ptr<ICompute> createComputeD3D() {
    return std::make_unique<Compute>();
}

}