#include <renderer/renderer.h>
#include <app/app.h>
#include <utils/utils.h>
#include <utilsD3D/utilsD3D.h>
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3dx12.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <directxmath.h>
#include <dxgi1_6.h>
#include <windows.h>
#include <wrl.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>
using namespace DirectX;
using namespace Microsoft::WRL;

namespace gpu {


class Executor : public IExecutor {
public:
    bool init(void* data) {
        GUARD(factory.init());
        factory.print();
        return true;
    }

    void terminate() {
    }

private:
    Factory factory;
};

std::unique_ptr<IExecutor> createExecutor() {
    return std::make_unique<Executor>();
}

}
