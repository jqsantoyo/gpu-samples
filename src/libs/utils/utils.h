#pragma once
#include <memory>
#include <string>
#include <vector>

namespace gpu {
    

struct uvec2 { uint32_t x, y;       };
struct uvec3 { uint32_t x, y, z;    };
struct uvec4 { uint32_t x, y, z, w; };
struct ivec2 { int32_t  x, y;       };
struct ivec3 { int32_t  x, y, z;    };
struct ivec4 { int32_t  x, y, z, w; };
struct  vec2 { float    x, y;       };
struct  vec3 { float    x, y, z;    };
struct  vec4 { float    x, y, z, w; };


std::wstring getAssetsPathW();
std::string getAssetsPath();
uint32_t align256(uint32_t x);
bool readFile(const char* filename, std::vector<uint8_t>& data);


class IExecutor {
public:
    virtual ~IExecutor() = default;
    virtual bool init(void* data) = 0;
    virtual void terminate() = 0;
    virtual bool execute() { return false; }
};

std::unique_ptr<IExecutor> createExecutor();

}