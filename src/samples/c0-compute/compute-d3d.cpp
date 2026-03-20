#include "compute.h"


namespace gpu {

class Compute : public ICompute {
public:
    bool init(int argc, char** argv) {
        return true;
    }
    void terminate() {
    }

private:



};

std::unique_ptr<ICompute> createCompute() {
    return std::make_unique<Compute>();
}

}