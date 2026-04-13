#include "scene.h"
#include <map>





namespace gpu {



bool Cameras::init(int capacity) {
    cameraPos.reserve(capacity);
    cameras.reserve(capacity);
    return true;
}

void Cameras::reset() {
    indexMap.clear();
    cameraPos.clear();
    cameras.clear();
}

int Cameras::size() {
    return cameras.size();
}

int Cameras::add(const std::string& name) {
    int idx = cameras.size();
    auto [it, result] = indexMap.insert({name, idx});
    if (result) {
        cameras.push_back({});
        cameraPos.push_back({});
        return idx;
    } else {
        return -1;
    }
}

int Cameras::getIdx(const std::string& name) {
    auto it = indexMap.find(name);
    if (it != indexMap.end()) {
        return it->second;
    } else {
        return -1;
    }
}

CameraPos* Cameras::getCameraPos(int idx) {
    return &cameraPos[idx];
}

Camera* Cameras::getCamera(int idx) {
    return &cameras[idx];
}



bool Lights::init(int capacity) {
    lights.reserve(capacity);
    return true;
}

void Lights::reset() {
    indexMap.clear();
    lights.clear();
}

int Lights::size() {
    return lights.size();
}

int Lights::add(const std::string& name) {
    int idx = lights.size();
    auto [it, result] = indexMap.insert({name, idx});
    if (result) {
        lights.push_back({});
        return idx;
    } else {
        return -1;
    }
}

int Lights::getIdx(const std::string& name) {
    auto it = indexMap.find(name);
    if (it != indexMap.end()) {
        return it->second;
    } else {
        return -1;
    }
}

Light* Lights::getLight(int idx) {
    return &lights[idx];
}



bool Objects::init(int capacity) {
    trs.reserve(capacity);
    transforms.reserve(capacity);
    models.reserve(capacity);
    return true;
}

void Objects::reset() {
    indexMap.clear();
    trs.clear();
    transforms.clear();
    models.clear();
}

int Objects::size() {
    return trs.size();
}

int Objects::add(const std::string& name) {
    int idx = trs.size();
    auto [it, result] = indexMap.insert({name, idx});
    if (result) {
        trs.push_back({});
        transforms.push_back({});
        models.push_back({});
        return idx;
    } else {
        return -1;
    }
}

int Objects::getIdx(const std::string& name) {
    auto it = indexMap.find(name);
    if (it != indexMap.end()) {
        return it->second;
    } else {
        return -1;
    }
}

Trs* Objects::getTrs(int idx) {
    return &trs[idx];
}
mat4* Objects::getTransform(int idx) {
    return &transforms[idx];
}

Model* Objects::getModel(int idx) {
    return &models[idx];
}



bool Scene::init(int cameraCapacity, int lightsCapacity, int objectCapacity) {
    cameras.init(cameraCapacity);
    lights.init(lightsCapacity);
    objects.init(objectCapacity);
    return true;
}

void Scene::reset() {
    cameras.reset();
    lights.reset();
    objects.reset();
}


int Scene::addCamera(const std::string name, float r, float theta, float phi, float fovY, float aspect, float nearZ, float farZ) {
    int idx = cameras.add(name);
    if (idx != -1) {
        CameraPos* cameraPos = cameras.getCameraPos(idx);
        Camera* camera = cameras.getCamera(idx);
        *cameraPos = { .r = r, .theta = theta, .phi = phi, };
        *camera = {
            .pos    = spherical2Cartesian(r, theta, phi),
            .target = { 0, 0, 0 },
            .up     = { 0, 1, 0 },
            .fovY   = fovY,
            .aspect =  aspect,
            .nearZ  = nearZ,
            .farZ   = farZ
        };
    }
    return idx;
}

int Scene::addObject(const std::string name, vec3 position, vec4 rotation, vec3 scale, int meshId, int materialId) {
    int idx = objects.add(name);
    if (idx != -1) {
        Model* model = objects.getModel(idx);
        Trs* trs = objects.getTrs(idx);
        *model = { meshId, materialId };
        *trs = { position, rotation, scale };
    }
    return idx;
}

}