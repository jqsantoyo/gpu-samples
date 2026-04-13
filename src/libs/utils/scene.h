#pragma once
#include <utils/renderer.h>
#include <memory>
#include <string>
#include <map>

namespace gpu {

struct CameraPos {
    float r = 20;
    float theta = 0;
    float phi = 0;
};

class Cameras {
public:
    bool       init(int capacity);
    void       reset();
    int        size();
    int        add(const std::string& name);
    int        getIdx(const std::string& name);
    CameraPos* getCameraPos(int idx = 0);
    Camera*    getCamera(int idx = 0);
private:
    std::vector<CameraPos>     cameraPos;
    std::vector<Camera>        cameras;
    std::map<std::string, int> indexMap;
};

class Lights {
public:
    bool    init(int capacity);
    void    reset();
    int     size();
    int     add(const std::string& name);
    int     getIdx(const std::string& name);
    Light*  getLight(int idx = 0);
private:
    std::vector<Light>         lights;
    std::map<std::string, int> indexMap;
};


class Objects {
public:
    bool    init(int capacity);
    void    reset();
    int     size();
    int     add(const std::string& name);
    int     getIdx(const std::string& name);
    Trs*    getTrs(int idx = 0);
    mat4*   getTransform(int idx = 0);
    Model*  getModel(int idx = 0);
private:
    std::vector<Trs>           trs;
    std::vector<mat4>          transforms;
    std::vector<Model>         models;
    std::map<std::string, int> indexMap;
};


class Scene {
public:
    Objects objects;
    Cameras cameras;
    Lights lights;
    bool init(int cameraCapacity, int lightsCapacity, int objectCapacity);
    void reset();

    int addCamera(const std::string name, float r, float theta, float phi, float fovY, float aspect, float nearZ, float farZ);
    int addObject(const std::string name, vec3 position, vec4 rotation, vec3 scale, int meshId, int materialId);
};

}