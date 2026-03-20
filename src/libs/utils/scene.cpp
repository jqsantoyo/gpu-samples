#include "scene.h"






namespace gpu {
    
class Scene : public IScene {
public:
    int addObject(ObjectDesc& desc) {
        objects.push_back(desc);
        items.push_back({
            .position   = { desc.position[0], desc.position[1], desc.position[2] },
            .scale      = { desc.scale[0], desc.scale[1], desc.scale[2] },
            .rotation   = { desc.rotation[0], desc.rotation[1], desc.rotation[2], desc.rotation[3] },
            .meshId     = desc.meshId,
            .materialId = desc.materialId,
        });

        return objects.size() - 1;
    }

    std::vector<RenderItem>& get() {
        return items;
    }

private:
    std::vector<ObjectDesc> objects;
    std::vector<RenderItem> items;
};


std::unique_ptr<IScene> createScene() {
    return std::make_unique<Scene>();
}

}