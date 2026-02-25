#include <utils/utils.h>
#include <memory>


int main(int argc, char** argv) {
    std::unique_ptr<gpu::IExecutor> executor = gpu::createExecutor();
    int result = executor->init(nullptr) ? 0 : -1;
    executor->terminate();
    return result;
}