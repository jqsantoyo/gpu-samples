#include "compute.h"
#include <chrono>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
using namespace gpu;

void fill(int count, vec3* a, vec3* b, vec3* c) {
    for (int i = 0; i < count; i++) {
        float v = static_cast<float>(i);
        a[i] = {  0, v, 2 * v };
        b[i] = { -v, v, 3 * v };
        c[i] = {  0, 0,     0 };
    }
}

void cpuCompute(int count, const vec3* a, const vec3* b, vec3* c) {
    for (int i = 0; i < count; i++) {
        const vec3& aa = a[i];
        const vec3& bb = b[i];
        c[i] = { aa.x + bb.x, aa.y + bb.y, aa.z + bb.z };
    }
}

void printResults(const char* label, float ms, int count, const vec3* c) {
    printf("%s ------------------------------ %f ms\n", label, ms);
    for (int i = 0; i < 10; i++) {
        printf("    c[%d] = { %5.1f, %5.1f, %5.1f }\n", i, c[i].x, c[i].y, c[i].z );
    }
}


int main(int argc, char** argv) {
    bool useVulkan = argc >= 2 && strcmp(argv[1], "-vk") == 0;
    printf("c0-compute %s\n\n", useVulkan ? "-vk" : "");
    std::unique_ptr<ICompute> compute = useVulkan ? createComputeD3D() : createComputeVk();
    GMAIN(compute->init());
    
    TimePoint t0, t1;
    float msCpu, msGpu;
    int n = 150001;
    std::vector<vec3> a(n);
    std::vector<vec3> b(n);
    std::vector<vec3> c(n);
    printf("\nNumber of elements: %d\n\n", n);

    fill(n, a.data(), b.data(), c.data());
    t0 = Clock::now();
    GMAIN(compute->compute(n, a.data(), b.data(), c.data()));
    t1 = Clock::now();
    msGpu = std::chrono::duration<float>(t1 - t0).count() * 1000.0f;
    printResults("GPU", msGpu, n, c.data());

    fill(n, a.data(), b.data(), c.data());
    t0 = Clock::now();
    cpuCompute(n, a.data(), b.data(), c.data());
    t1 = Clock::now();
    msCpu = std::chrono::duration<float>(t1 - t0).count() * 1000.0f;
    printResults("CPU", msCpu, n, c.data());

    printf("\nGPU/CPU time ratio is %f\n\n", msGpu/msCpu);


    compute->terminate();
    return 0;
}
