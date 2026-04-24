#pragma once
#include <utils/utils.h>
#include <memory>

namespace gpu {

class ICompute {
public:
    virtual ~ICompute() = default;
    virtual bool init() = 0;
    virtual bool compute(int count, vec3* a, vec3* b, vec3* c) = 0;
    virtual void terminate() = 0;
};

std::unique_ptr<ICompute> createComputeD3D();
std::unique_ptr<ICompute> createComputeVk();
}