#pragma once
#include <utils/utils.h>
#include <app/app.h>
#include <scene/scene.h>
#include <rendererInterface/renderer.h>
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