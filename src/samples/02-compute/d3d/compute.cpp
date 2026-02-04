#include "../compute.h"


namespace gpu {

class Compute : public ICompute {
public:
    int start(int argc, char** argv) {
        return 1;
    }
    int stop() {
        return 1;
    }

private:



};

std::unique_ptr<ICompute> createCompute() {
    return std::make_unique<Compute>();
}

}