#pragma once
#include <memory>

namespace gpu {

class ICompute {
public:
    virtual ~ICompute() = default;
    virtual int start(int argc, char** argv) = 0;
    virtual int stop() = 0;
};

std::unique_ptr<ICompute> createCompute();
}