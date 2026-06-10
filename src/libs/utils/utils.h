#pragma once
#include <memory>
#include <string>
#include <vector>

namespace gpu {

#define GMAIN(x)              if (!(x)) {  printf("Error: "#x"\n"); return 0; }
#define GUARD(x)              if (!(x)) {  printf("Error: "#x"\n"); return false; }
#define GUARDM(x, fmt, ...)   if (!(x)) {  printf("Error: " fmt "\n", ##__VA_ARGS__);  return false; }

struct uvec2 { uint32_t x, y;       };
struct uvec3 { uint32_t x, y, z;    };
struct uvec4 { uint32_t x, y, z, w; };
struct ivec2 { int32_t  x, y;       };
struct ivec3 { int32_t  x, y, z;    };
struct ivec4 { int32_t  x, y, z, w; };
struct  vec2 { float    x, y;       };
struct  vec3 { float    x, y, z;    };
struct  vec4 { float    x, y, z, w; };
struct  mat4 { float    v[16]; };
struct Trs {
    vec3 position;
    vec4 rotation;
    vec3 scale;
};

uint32_t toUint(vec4 v);



template<typename Id, typename Data>
class Vec : public std::vector<Data> {
public:

    void init(int size) {
        this->resize(size);
    }

    Id add() {
        int idx = this->size();
        this->push_back(Data{});
        return { idx };
    }

    Data& operator[](Id id){
        return this->data()[id.idx];
    }

    Data& operator[](int idx){
        return this->data()[idx];
    }
};



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

vec3 spherical2Cartesian(float r, float theta, float phi);


}