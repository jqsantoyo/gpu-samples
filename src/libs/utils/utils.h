#pragma once
#include <memory>
#include <string>

namespace gpu {

std::wstring getAssetsPathW();
std::string getAssetsPath();
uint32_t align256(uint32_t x);


class IExecutor {
public:
    virtual bool init(void* data) = 0;
    virtual void terminate() = 0;
    virtual bool execute() { return false; }
};

std::unique_ptr<IExecutor> createExecutor();

}