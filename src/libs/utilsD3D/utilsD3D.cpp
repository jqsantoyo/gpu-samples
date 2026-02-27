#include "utilsD3D.h"
#include <utils/utils.h>
using namespace DirectX;
using namespace Microsoft::WRL;


namespace gpu {


bool Factory::init() {
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    
    GUARDHR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));

    int i = 0;
    IDXGIAdapter* adapter = nullptr;
    while(factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
        adapters.push_back({});
        Adapter& a = adapters.back();

        a.obj = adapter;
        adapter->GetDesc(&a.desc);
        
        int j = 0;
        IDXGIOutput* output = nullptr;
        while(adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND) {
            a.outputs.push_back({});
            Output& o = a.outputs.back();

            o.obj = output;
            output->GetDesc(&o.desc);
            UINT modeCount = 0;
            output->GetDisplayModeList(format, 0, &modeCount, nullptr);
            o.modes.resize(modeCount);
            output->GetDisplayModeList(format, 0, &modeCount, &o.modes[0]);
            j++;
        }
        i++;
    }
    return true;
}

void Factory::print() {
    printf("\nAdapters: %zu\n", adapters.size());
    for (auto& a : adapters) {
        printf("Adapter\n");
        printf("  Description: %ls\n",              a.desc.Description);
        printf("  VendorId: %d\n",                  a.desc.VendorId);
        printf("  DeviceId: %d\n",                  a.desc.DeviceId);
        printf("  SubSysId: %d\n",                  a.desc.SubSysId);
        printf("  Revision: %d\n",                  a.desc.Revision);
        printf("  DedicatedVideoMemory: %zu\n",     a.desc.DedicatedVideoMemory);
        printf("  DedicatedSystemMemory: %zu\n",    a.desc.DedicatedSystemMemory);
        printf("  SharedSystemMemory: %zu\n",       a.desc.SharedSystemMemory);
        for (auto& o : a.outputs) {
            printf("    Output: %ls\n", o.desc.DeviceName);
            for (auto m : o.modes) {
                printf("      resolution: %d x %d, refresh: %d / %d\n", m.Width, m.Height, m.RefreshRate.Numerator, m.RefreshRate.Denominator);
            }
        }
    }
}


bool Shader::compile(std::wstring dir, std::wstring name, const char* entry, const char* target, uint32_t flags) {
    std::wstring path = getAssetsPathW() + dir + L"\\" + name;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT res = D3DCompileFromFile(path.c_str(), nullptr, nullptr, entry, target, flags, 0, &blob, &errorBlob);
    if (FAILED(res)) {
        if (errorBlob) {
            const char* msg = static_cast<const char*>(errorBlob->GetBufferPointer());
            printf("%s\n", msg);
        } else {
            printf("D3DCompileFromFile[%ls]: %ld\n", path.c_str(), res);
        }
        return false;
    }
    bytecode = { blob->GetBufferPointer(), blob->GetBufferSize() };
    return true;
}

bool Shader::load(std::string dir, std::string name) {
    bool result = false;
    std::string path = getAssetsPath() + dir + "\\" + name;
    FILE* file;
    errno_t err = fopen_s(&file, path.c_str(), "rb");
    if (err != 0 || file == nullptr) {
        return result;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    HRESULT res = D3DCreateBlob(size, blob.GetAddressOf());
    if (SUCCEEDED(res)) {
        fread(blob->GetBufferPointer(), size, 1, file);
        bytecode = { blob->GetBufferPointer(), blob->GetBufferSize() };
        result = true;
    }
    fclose(file);
    return result;
}







}