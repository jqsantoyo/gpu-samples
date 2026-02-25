#include <renderer/renderer.h>
#include <app/app.h>
#include <utils/utils.h>
#include <utilsVk/utilsVk.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <inttypes.h>

namespace gpu {

class Executor : public IExecutor {
public:
    bool init(void* data) {
        GUARD(instance.init("00-info-vk", VK_MAKE_VERSION(1, 0, 0), false, {}));
        GUARD(getPhysicalDevices(instance.instance, phyDevices));

        printf("\nPhysical devices: %zu\n", phyDevices.size());
        for (int i = 0; i < phyDevices.size(); i++) {
            printf("[%d]:\n", i);
            phyDevices[i].print();
        }

        return true;
    }

    void terminate() {
        instance.terminate();
    }

private:
    Instance instance;
    std::vector<PhysicalDeviceData> phyDevices;
};

std::unique_ptr<IExecutor> createExecutor() {
    return std::make_unique<Executor>();
}

}
