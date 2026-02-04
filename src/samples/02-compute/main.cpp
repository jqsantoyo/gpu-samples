#include "compute.h"



int main(int argc, char** argv) {
    std::unique_ptr<gpu::ICompute> compute = gpu::createCompute();
    if (!compute->start(argc, argv)) {
        printf("compute::start error\n");
    }
    compute->stop();
    return 0;
}
