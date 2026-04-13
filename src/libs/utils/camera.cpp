#include "camera.h"
#include <algorithm>
#include <utils/utils.h>

namespace gpu {


void CameraCtrl::mouseEvent(MouseEvent event, int count, CameraPos* cameraPos, Camera* cameras) {
    cameraPos += selectedCamera;
    cameras += selectedCamera;
    bool dirty = false;
    switch(event.type) {
    case Move:
        // printf("Camera:: mouse moved %d, %d\n", event.x, event.y);
        if (mouseActive) {
            float halfPi = 3.14159 / 2;
            float s = 0.01f;
            float dTheta = s * (mouseX - event.x);
            float dPhi = -s * (mouseY - event.y);
            cameraPos->theta += dTheta;
            cameraPos->phi += dPhi;
            cameraPos->phi = std::clamp(cameraPos->phi, -halfPi, halfPi);
            mouseX = event.x;
            mouseY = event.y;
            dirty = true;
        }
        break;
    case Wheel:
        // printf("Camera:: mouse wheel %d\n", event.wheel);
        cameraPos->r += -static_cast<float>(event.wheel) / 60.0f;
        cameraPos->r = std::clamp(cameraPos->r, 5.0f, 100.0f);
        dirty = true;
        break;
    case Button:
        // printf("Camera:: mouse button: %d, %d\n", event.button, event.down);
        if (event.button == Left || event.button == Middle) {
            mouseActive = event.down;
            mouseX = event.x;
            mouseY = event.y;
        }
        break;
    }
    if (dirty) {
        // printf("Camera: %f, %f, %f, <%f, %f, %f>\n",
        //     cameraPos->r, cameraPos->theta, cameraPos->phi,
        //     cameras->pos.x, cameras->pos.y, cameras->pos.z
        // );
        *cameras = {
            .pos    = spherical2Cartesian(cameraPos->r, cameraPos->theta, cameraPos->phi),
            .target = { 0, 0, 0 },
            .up     = { 0, 1, 0 },
            .fovY   = 3.14159 / 4.0f,
            .aspect =  512.0f / 512.0f,
            .nearZ  = 0.1f,
            .farZ   = 200.0f
        };
    }
}

};