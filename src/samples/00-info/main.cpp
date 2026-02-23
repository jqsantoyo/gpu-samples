
namespace gpu {
    int info();
}

int main(int argc, char** argv) {
    if (gpu::info()) {
        return 0;
    } else {
        return -1;
    }
}