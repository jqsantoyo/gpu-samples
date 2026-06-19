#pragma once
#include <memory>
#include <string>
#include <app/app.h>
#include <scene/scene.h>
#include <rendererInterface/renderer.h>
#include <functional>


namespace gpu {


class SceneLoader {
public:
    void init    (Scene* scene, IRenderer* renderer);
    void reset   ();
    void record  (Buffer buffer);
    void record  (Mesh mesh);
    void record  (MaterialTexture materialTexture);
    void record  (Material material);
    bool load    (const std::string& filename);
private:
    Scene* scene;
    IRenderer* renderer;
    std::vector<Buffer>             buffers;
    std::vector<Mesh>               meshes;
    std::vector<MaterialTexture>    materialTextures;
    std::vector<Material>           materials;
};


class SceneSelector {
public:
    void init(SceneLoader* sceneLoader, std::initializer_list<std::function<bool()>> loaders);
    bool load(int idx);
    bool loadOnKeyboard(KeyboardEvent event);
private:
    SceneLoader*                             sceneLoader;
    std::vector<std::function<bool()>>       loaders;
    int                                      sceneIdx = 0;
};


}