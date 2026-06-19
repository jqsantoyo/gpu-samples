#include "sceneLoader.h"
#include <utils/utils.h>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include <tiny_gltf.h>
#include <string>
#include <algorithm>


namespace gpu {

struct LoaderContext {
    SceneLoader* sceneLoader;
    IRenderer*   renderer;
    std::string  filename;
    std::vector<MaterialTexture> materialTextureMap;
};

bool loadImage(
    tinygltf::Image* image,
    const int imageIdx,
    std::string* error,
    std::string* warning,
    int reqWidth,
    int reqHeight,
    const unsigned char* bytes,
    int size,
    void* userData
) {
    LoaderContext* ctx = reinterpret_cast<LoaderContext*>(userData);
    std::string name = ctx->filename + ":" + image->name + ":" + std::to_string(imageIdx);
    MaterialTexture matTex = ctx->renderer->create(name.c_str(), bytes, size);
    if (ctx->materialTextureMap.size() < imageIdx + 1) {
        ctx->materialTextureMap.resize(imageIdx + 1);
    }
    ctx->materialTextureMap[imageIdx] = matTex;
    ctx->sceneLoader->record(matTex);
    return true;
}

void SceneLoader::init(Scene* scene, IRenderer* renderer) {
    this->scene = scene;
    this->renderer = renderer;
}

void SceneLoader::reset() {
    renderer->wait();
    renderer->reset(); // erases all meshes
    scene->reset();
    for (int i = 0; i < buffers.size(); i++) {
        renderer->destroy(buffers[i]);
    }
    for (int i = 0; i < materialTextures.size(); i++) {
        renderer->destroy(materialTextures[i]);
    }
    for (int i = 0; i < materials.size(); i++) {
        renderer->destroy(materials[i]);
    }
    buffers.clear();
    buffers.clear();
    materialTextures.clear();
    materials.clear();
}

void SceneLoader::record(Buffer buffer) {
    buffers.push_back(buffer);
}

void SceneLoader::record(Mesh mesh) {
    meshes.push_back(mesh); // not needed, since MeshRegistry will reset all
}

void SceneLoader::record(MaterialTexture materialTexture) {
    materialTextures.push_back(materialTexture);
}

void SceneLoader::record(Material material) {
    materials.push_back(material);
}

bool SceneLoader::load(const std::string& filename) {
    std::string path = getAssetsPath() + filename;
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    LoaderContext ctx = { this, renderer, filename, {} };
    loader.SetImageLoader(loadImage, &ctx); 
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
                           
    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }
    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }
    if (!ret) {
        printf("Failed to parse glTF: %s\n", path.c_str());
    }

    // Reserve memory
    int cameraCount = 0;
    int lightCount  = 0;
    int objectCount = 0;
    for (int i = 0; i < model.nodes.size(); i++) {
        const tinygltf::Node& n = model.nodes[i];
        if (n.camera >= 0) {
            cameraCount++;
        }
        if (n.light >= 0) {
            lightCount++;
        }
        if (n.mesh >= 0) {
            objectCount++;
        }
    }
    std::vector<MaterialTexture>& materialTextureMap = ctx.materialTextureMap;
    std::vector<Buffer>             bufferMap           (model.buffers.size(),   {-1});
    std::vector<Mesh>               meshMap             (model.meshes.size(),    {-1});
    std::vector<Material>           materialMap         (model.materials.size(), {-1});

    cameraCount = std::max(cameraCount, 1); // we still create a default camera if none was found
    scene->init(cameraCount, lightCount, objectCount);

    int bufferId = 0; // expceted only 1 buffer
    for (int i = 0; i < model.buffers.size(); i++) {
        const tinygltf::Buffer& buffer = model.buffers[i];
        printf("Assets:: model[%s]::buffer[%d]: size: %zu\n", filename.c_str(), i, buffer.data.size());
        std::string name = filename + ":" + buffer.name + ":" + std::to_string(i);
        Buffer buff = renderer->create(name.c_str(), buffer.data.size(), buffer.data.data());
        bufferMap[i] = buff;
        record(buff);
    }

    printf("Assets:: meshes: %zu\n", model.meshes.size());
    for (int i = 0; i < model.meshes.size(); i++) {
        const tinygltf::Mesh& m = model.meshes[i];
        const tinygltf::Primitive& prim = m.primitives[0];

        tinygltf::BufferView indicesView;
        tinygltf::BufferView positionView;
        tinygltf::BufferView normalView;
        tinygltf::BufferView uvView;
        tinygltf::BufferView tangentView;
        tinygltf::BufferView colorView;
        auto positionAttr   = prim.attributes.find("POSITION");
        auto normalAttr     = prim.attributes.find("NORMAL");
        auto uvAttr         = prim.attributes.find("TEXCOORD_0");
        auto tangentAttr    = prim.attributes.find("TANGENT");
        auto colorAttr      = prim.attributes.find("COLOR_0");
        ASSERT(positionAttr != prim.attributes.end());

        tinygltf::Accessor& indicesAcc = model.accessors[prim.indices];
        tinygltf::Accessor& positionAcc = model.accessors[positionAttr->second];
        indicesView = model.bufferViews[indicesAcc.bufferView];
        positionView = model.bufferViews[positionAcc.bufferView];
        Format format = indicesAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? Format::R16u : Format::R32u;
        ASSERT(indicesAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT || indicesAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);
        ASSERT(positionAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && positionAcc.type == TINYGLTF_TYPE_VEC3);
        ASSERT(indicesView.byteStride == 0);
        ASSERT(positionView.byteStride == 0);
        size_t vCount = indicesAcc.count;

        if (normalAttr != prim.attributes.end()) {
            tinygltf::Accessor& acc = model.accessors[normalAttr->second];
            normalView = model.bufferViews[acc.bufferView];
            ASSERT(acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && acc.type == TINYGLTF_TYPE_VEC3);
            ASSERT(normalView.byteStride == 0);
        }

        if (uvAttr != prim.attributes.end()) {
            tinygltf::Accessor& acc = model.accessors[uvAttr->second];
            uvView = model.bufferViews[acc.bufferView];
            ASSERT(acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && acc.type == TINYGLTF_TYPE_VEC2);
            ASSERT(uvView.byteStride == 0);
        }

        if (tangentAttr != prim.attributes.end()) {
            tinygltf::Accessor& acc = model.accessors[tangentAttr->second];
            tangentView = model.bufferViews[acc.bufferView];
            ASSERT(acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && acc.type == TINYGLTF_TYPE_VEC4);
            ASSERT(tangentView.byteStride == 0);
        }

        if (colorAttr != prim.attributes.end()) {
            tinygltf::Accessor& acc = model.accessors[colorAttr->second];
            colorView = model.bufferViews[acc.bufferView];
            ASSERT(acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && acc.type == TINYGLTF_TYPE_VEC3);
            ASSERT(colorView.byteStride == 0);
        }


        std::string key = filename + m.name;
        Buffer buffer = bufferId != -1 ? bufferMap[bufferId] : Buffer{ -1 };
        MeshData meshDesc = {
            .vCount = vCount,
            .indices  = { buffer,  indicesView.byteOffset, static_cast<uint32_t>( indicesView.byteLength), format },
            .position = { buffer, positionView.byteOffset, static_cast<uint32_t>(positionView.byteLength), sizeof(float) * 3 },
            .normal   = { buffer,   normalView.byteOffset, static_cast<uint32_t>(  normalView.byteLength), sizeof(float) * 3 },
            .uv       = { buffer,       uvView.byteOffset, static_cast<uint32_t>(      uvView.byteLength), sizeof(float) * 2 },
            .tangent  = { buffer,  tangentView.byteOffset, static_cast<uint32_t>( tangentView.byteLength), sizeof(float) * 4 },
            .color    = { buffer,    colorView.byteOffset, static_cast<uint32_t>(   colorView.byteLength), sizeof(float) * 3 },
        };
        Mesh mesh = renderer->create(meshDesc);
        meshMap[i] = mesh;
        record(mesh);
    }

    printf("Assets:: materials: %zu\n", model.materials.size());
    for (int i = 0; i < model.materials.size(); i++) {
        const tinygltf::Material& m = model.materials[i];
        std::string name = filename + ":" + m.name + ":" + std::to_string(i);
        int baseIdx      = m.pbrMetallicRoughness.baseColorTexture.index;
        int ormIdx       = m.pbrMetallicRoughness.metallicRoughnessTexture.index;
        int normalIdx    = m.normalTexture.index;
        int emissiveIdx  = m.emissiveTexture.index;
        MaterialTexture mapBaseColor    = baseIdx     != -1 ? materialTextureMap[model.textures[baseIdx].source]     : MaterialTexture{ -1 };
        MaterialTexture mapOrm          = ormIdx      != -1 ? materialTextureMap[model.textures[ormIdx].source]      : MaterialTexture{ -1 };
        MaterialTexture mapNormal       = normalIdx   != -1 ? materialTextureMap[model.textures[normalIdx].source]   : MaterialTexture{ -1 };
        MaterialTexture mapEmissive     = emissiveIdx != -1 ? materialTextureMap[model.textures[emissiveIdx].source] : MaterialTexture{ -1 };

        MaterialDesc desc = {
            .baseColor = {
                static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[0]),
                static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[1]),
                static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[2]),
                static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[3]),
            },
            .emissive = {
                static_cast<float>(m.emissiveFactor[0]),
                static_cast<float>(m.emissiveFactor[1]),
                static_cast<float>(m.emissiveFactor[2]),
            },
            .metallic               = static_cast<float>(m.pbrMetallicRoughness.metallicFactor),
            .roughness              = static_cast<float>(m.pbrMetallicRoughness.roughnessFactor),
            .baseColorMap           = mapBaseColor,
            .ormMap                 = mapOrm,
            .normalMap              = mapNormal,
            .emissiveMap            = mapEmissive,
        };
        Material material = renderer->create(name.c_str(), desc);
        materialMap[i] = material;
        record(material);
    }

    printf("Assets:: objects: %zu\n", model.nodes.size());
    for (int i = 0; i < model.nodes.size(); i++) {
        const tinygltf::Node& n = model.nodes[i];
        std::string name = "object:" + std::to_string(i);
        // float x = n.translation.size() > 0 ? static_cast<float>(n.translation[0]) : 0;
        // float y = n.translation.size() > 0 ? static_cast<float>(n.translation[1]) : 0;
        // float z = n.translation.size() > 0 ? static_cast<float>(n.translation[2]) : 0;
        float x = 0, y = 0, z = 0;
        float sx = 1, sy = 1, sz = 1;
        float rx = 0, ry = 0, rz = 0, rw = 1;
        if (n.translation.size() > 0) {
            x = static_cast<float>(n.translation[0]);
            y = static_cast<float>(n.translation[1]);
            z = static_cast<float>(n.translation[2]);
        }
        if (n.scale.size() > 0) {
            sx = static_cast<float>(n.scale[0]);
            sy = static_cast<float>(n.scale[1]);
            sz = static_cast<float>(n.scale[2]);
        }
        if (n.rotation.size() > 0) {
            rx = static_cast<float>(n.rotation[0]);
            ry = static_cast<float>(n.rotation[1]);
            rz = static_cast<float>(n.rotation[2]);
            rw = static_cast<float>(n.rotation[3]);
        }
        if (n.mesh >= 0) {
            const tinygltf::Mesh&       mesh     = model.meshes[n.mesh];
            const tinygltf::Primitive&  prim     = mesh.primitives[0];
            Mesh                        meshId   = meshMap[n.mesh];
            Material                    material = prim.material != -1 ? materialMap[prim.material] : Material{ -1 };
            int objectIdx = scene->addObject(n.name, { x, y, z }, { rx, ry, rz, rw }, { sx, sy, sz }, meshId.idx, material.idx);
        }
    }

    scene->addCamera("defaultCamera", 6, -2.1, .25, 3.14159 / 4.0f, 1, 0.1f, 200.0f);
    return true;
}



void SceneSelector::init(SceneLoader* sceneLoader, std::initializer_list<std::function<bool()>> loaders) {
    this->sceneLoader = sceneLoader;
    this->loaders.assign(loaders.begin(), loaders.end());
}

bool SceneSelector::load(int idx = 0) {
    sceneIdx = idx;
    return loaders[idx]();
}

bool SceneSelector::loadOnKeyboard(KeyboardEvent event) {
    if (event.press) {
        if (event.key == KeyRight) {
            sceneIdx++;
            if (sceneIdx >= loaders.size()) {
                sceneIdx = 0;
            }
        } else if (event.key == KeyLeft) {
            sceneIdx--;
            if (sceneIdx < 0) {
                sceneIdx = loaders.size() - 1;
            }
        } else {
            return false;
        }
        printf("Load scene %d.\n", sceneIdx);
        sceneLoader->reset();
        bool result = loaders[sceneIdx]();
        return result;
    } else {
        return true;
    }
}

}