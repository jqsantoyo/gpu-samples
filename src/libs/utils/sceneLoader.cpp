#include "sceneLoader.h"
#include <utils/utils.h>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include <tiny_gltf.h>
#include <string>
#include <algorithm>



namespace gpu {


void SceneLoader::init(Scene* scene, IRenderer* renderer) {
    this->scene = scene;
    this->renderer = renderer;
}

bool SceneLoader::load(const std::string& filename) {
    std::string path = getAssetsPath() + filename;
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
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
    cameraCount = std::max(cameraCount, 1); // we still create a default camera if none was found
    scene->init(cameraCount, lightCount, objectCount);

    int bufferId = 0; // expceted only 1 buffer
    for (int i = 0; i < model.buffers.size(); i++) {
        const tinygltf::Buffer& buffer = model.buffers[i];
        printf("Assets:: model[%s]::buffer[%d]: size: %zu\n", filename.c_str(), i, buffer.data.size());
        BufferDesc bufferDesc = {
            .offset = 0,
            .size = buffer.data.size(),
            .data = buffer.data.data(),
        };
        int bufferIdx = renderer->addBuffer(bufferDesc);
    }

    printf("Assets:: meshes: %zu\n", model.meshes.size());
    for (int i = 0; i < model.meshes.size(); i++) {
        const tinygltf::Mesh& m = model.meshes[i];
        const tinygltf::Primitive& prim = m.primitives[0];

        size_t vCount = 0;
        tinygltf::BufferView indicesView;
        tinygltf::BufferView positionView;
        tinygltf::BufferView normalView;
        tinygltf::BufferView uvView;
        tinygltf::BufferView colorView;
        auto positionAttr = prim.attributes.find("POSITION");
        auto normalAttr = prim.attributes.find("NORMAL");
        auto uvAttr = prim.attributes.find("TEXCOORD_0");
        auto colorAttr = prim.attributes.find("COLOR_0");
        if (positionAttr != prim.attributes.end()) {
            tinygltf::Accessor& indicesAcc = model.accessors[prim.indices];
            tinygltf::Accessor& positionAcc = model.accessors[positionAttr->second];
            if (indicesAcc.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                printf("Assets:: model[%s]::mesh[%d] indices are not u16", filename.c_str(), i);
                break;
            }
            if (positionAcc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
                positionAcc.type != TINYGLTF_TYPE_VEC3
            ) {
                printf("Assets:: model[%s]::mesh[%d] positions are not vec3f", filename.c_str(), i);
                break;
            }
            indicesView = model.bufferViews[indicesAcc.bufferView];
            positionView = model.bufferViews[positionAcc.bufferView];
            if (indicesView.byteStride != 0) {
                printf("Assets:: model[%s]::mesh[%d] indices has invalid stride", filename.c_str(), i);
                break;
            }
            if (positionView.byteStride != 0) {
                printf("Assets:: model[%s]::mesh[%d] position has invalid stride", filename.c_str(), i);
                break;
            }
            vCount = indicesAcc.count;
        } else {
            printf("Assets:: model[%s]::mesh[%d] missing position attribute", filename.c_str(), i);
        }

        if (normalAttr != prim.attributes.end()) {
            tinygltf::Accessor& acc = model.accessors[normalAttr->second];
            if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
                acc.type != TINYGLTF_TYPE_VEC3
            ) {
                printf("Assets:: model[%s]::mesh[%d] normals are not vec3f", filename.c_str(), i);
                break;
            }
            normalView = model.bufferViews[acc.bufferView];
            if (normalView.byteStride != 0) {
                printf("Assets:: model[%s]::mesh[%d] normal has invalid stride", filename.c_str(), i);
                break;
            }
        }

        if (uvAttr != prim.attributes.end()) {
            tinygltf::Accessor& acc = model.accessors[uvAttr->second];
            if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
                acc.type != TINYGLTF_TYPE_VEC2
            ) {
                printf("Assets:: model[%s]::mesh[%d] uvs are not vec2f", filename.c_str(), i);
                break;
            }
            uvView = model.bufferViews[acc.bufferView];
            if (uvView.byteStride != 0) {
                printf("Assets:: model[%s]::mesh[%d] uv has invalid stride", filename.c_str(), i);
                break;
            }
        }

        if (colorAttr != prim.attributes.end()) {
            tinygltf::Accessor& acc = model.accessors[colorAttr->second];
            if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
                acc.type != TINYGLTF_TYPE_VEC3
            ) {
                printf("Assets:: model[%s]::mesh[%d] colors are not vec3f", filename.c_str(), i);
                break;
            }
            colorView = model.bufferViews[acc.bufferView];
            if (colorView.byteStride != 0) {
                printf("Assets:: model[%s]::mesh[%d] color has invalid stride", filename.c_str(), i);
                break;
            }
        }


        std::string key = filename + m.name;
        MeshDesc meshDesc = {
            .bufferId = bufferId,
            .vCount = vCount,
            .indices = {
                .offset = indicesView.byteOffset,
                .size = indicesView.byteLength,
            },
            .position = {
                .offset = positionView.byteOffset,
                .size = positionView.byteLength,
            },
            .normal = {
                .offset = normalView.byteOffset,
                .size = normalView.byteLength,
            },
            .uv = {
                .offset = uvView.byteOffset,
                .size = uvView.byteLength,
            },
            .color = {
                .offset = colorView.byteOffset,
                .size = colorView.byteLength,
            },
        };
        int meshIdx = renderer->addMesh(meshDesc);
    }


    printf("Assets:: objects: %zu\n", model.nodes.size());
    for (int i = 0; i < model.nodes.size(); i++) {
        const tinygltf::Node& n = model.nodes[i];
        std::string name = std::string("") + "object_" + std::to_string(i);
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
        int objectIdx = scene->addObject(n.name, { x, y, z }, { rx, ry, rz, rw }, { sx, sy, sz }, n.mesh, 0);
    }

    scene->addCamera("defaultCamera", 20, 0, 0, 3.14159 / 4.0f, 1, 0.1f, 100.0f);
    return true;
}
}