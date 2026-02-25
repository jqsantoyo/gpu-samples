#include "compute.h"



int main(int argc, char** argv) {
    std::unique_ptr<gpu::ICompute> compute = gpu::createCompute();
    if (!compute->init(argc, argv)) {
        printf("compute::start error\n");
    }
    compute->terminate();
    return 0;
}
