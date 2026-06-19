#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <cassert>

namespace gpu {

#define ASSERT(x)             { assert(x); }
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



class FreeList {
public:
    void    init(int size);
    int     alloc();
    void    reset();
    void    free(int idx);
    size_t  size();
private:
    size_t              count = 0;
    int                 nextFree = 0;
    std::vector<int>    freeList;
    std::vector<bool>   allocated;
};


template<typename Id, typename Data>
class Vec {
public:

    Vec() {}
    
    void init(uint32_t size) {
        freeList.init(size);
        vector.resize(size);
        names.resize(size);
        map.reserve(size);
    }

    void reset() {
        int size = vector.size();
        vector.clear();
        names.clear();
        map.clear();
        vector.resize(size);
        names.resize(size);
        map.reserve(size);
        freeList.reset();
    }

    size_t size() {
        return freeList.size();
    }

    Id alloc(const std::string& name) {
        auto it = map.find(name);
        ASSERT(it == map.end());
        int idx = freeList.alloc();
        names[idx] = name;
        std::string_view nameView = names[idx]; // MUST reference string in names vector!
        map[nameView] = { idx };
        return { idx };
    }

    Id alloc() {
        int idx = freeList.alloc();
        return { idx };
    }

    void free(Id id) {
        std::string_view key = names[id.idx];
        map.erase(key);     // may or may not find and erase the entry
        names[id.idx] = "";
        freeList.free(id.idx);
    }

    Id get(const std::string& name) {
        auto it = map.find(name);
        if (it != map.end()) {
            return it->second;
        } else {
            return { -1 };
        }
    }

    Data& operator[](Id id){
        return vector[id.idx];
    }

    Data& operator[](int idx){
        return vector[idx];
    }

private:
    FreeList                                    freeList;
    std::vector<Data>                           vector;
    std::unordered_map<std::string_view, Id>    map; // string_view as key avoids duplicating string
    std::vector<std::string>                    names;
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

void trs2Transform(int count, const Trs* trs, mat4* transforms);

}