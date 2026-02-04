#include "utilsD3D.h"
#include <utils/utils.h>
using namespace DirectX;
using namespace Microsoft::WRL;


namespace gpu {


bool compileShader(Shader& shader, std::wstring dir, std::wstring name, const char* entry, const char* target, uint32_t flags) {
    std::wstring path = getAssetsPathW() + dir + L"\\" + name;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT res = D3DCompileFromFile(path.c_str(), nullptr, nullptr, entry, target, flags, 0, &shader.blob, &errorBlob);
    if (FAILED(res)) {
        if (errorBlob) {
            const char* msg = static_cast<const char*>(errorBlob->GetBufferPointer());
            printf("%s\n", msg);
        } else {
            printf("D3DCompileFromFile[%ls]: %ld\n", path.c_str(), res);
        }
        return false;
    }
    shader.bytecode = { shader.blob->GetBufferPointer(), shader.blob->GetBufferSize() };
    return true;
}

bool loadShader(Shader& shader, std::string dir, std::string name) {
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
    
    HRESULT res = D3DCreateBlob(size, shader.blob.GetAddressOf());
    if (SUCCEEDED(res)) {
        fread(shader.blob->GetBufferPointer(), size, 1, file);
        shader.bytecode = { shader.blob->GetBufferPointer(), shader.blob->GetBufferSize() };
        result = true;
    }
    fclose(file);
    return result;
}


}