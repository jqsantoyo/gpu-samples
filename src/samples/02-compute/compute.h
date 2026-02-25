#pragma once
#include <memory>

namespace gpu {

class ICompute {
public:
    virtual ~ICompute() = default;
    virtual bool init(int argc, char** argv) = 0;
    virtual void terminate() = 0;
};

std::unique_ptr<ICompute> createCompute();
}