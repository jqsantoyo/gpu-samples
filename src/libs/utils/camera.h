#pragma once
#include <utils/utils.h>
#include <utils/app.h>
#include <utils/scene.h>
#include <utils/renderer.h>
#include <memory>

namespace gpu {


class CameraCtrl {
public:
    void mouseEvent(MouseEvent event, int count, CameraPos* cameraPos, Camera* cameras);
private:
    int selectedCamera = 0;
    bool mouseActive = false;
    int mouseX = 0;
    int mouseY = 0;
};

}