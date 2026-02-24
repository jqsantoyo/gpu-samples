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
#ifdef WIN32
#else
    #include <vulkan/vulkan_metal.h>
#endif

namespace gpu {

class RendererVk : public IRenderer {
public:
    bool init(void* window, uint32_t screenWidth, uint32_t screenHeight) {
        
        GUARD(instance.init("02-info-window", VK_MAKE_VERSION(1, 0, 0), true, {}, {}));
        // GUARD(surface.init(instance.instance, window));

        return true;
    }

    void terminate() {
        // vkDeviceWaitIdle(device);
        instance.deinit();
    }

    bool resize(int width, int height) {
        return true;
    }

    bool render(const Color& clearColor, const std::vector<RenderItem>& items) {
        return 1;
    }

private:
    Instance instance;
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererVk>();
}
}
