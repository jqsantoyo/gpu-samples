#include "camera.h"


namespace gpu {
class Camera : public ICamera {
public:
    void updateRadius(float dr) {
        r += dr;
        if (r <= 5) {
            r = 5;
        }
        if (r > 50) {
            r = 50;
        }
    }

    void updateTheta(float dTheta) {
        theta += dTheta;
    }

    void updatePhi(float dPhi) {
        phi += dPhi;
        if (phi >= halfPi) {
            phi = halfPi;
        }
    }

    void getCartesian(float& x, float& y, float& z) {
        printf("Camera:: theta: %f, phi: %f, z: %f\n", theta, phi, r);
        float h = r * cos(phi);
        x = h * cos(theta);
        z = h * sin(theta);
        y = r * sin(phi);
        // static float t = .1f;
        // t += .005f;
        // x = 4 * sin(t) + 8.0f;
        // y = -5 * sin(t) + 8.0f;
        // z = -10;
    }

    void mouseEvent(MouseEvent event) {
        switch(event.type) {
        case Move:
            // printf("Camera:: mouse moved %d, %d\n", event.x, event.y);
            if (mouseActive) {
                float s = 0.01f;
                float dTheta = s * (mouseX - event.x);
                float dPhi = -s * (mouseY - event.y);
                mouseX = event.x;
                mouseY = event.y;
                updateTheta(dTheta);
                updatePhi(dPhi);
            }
            break;
        case Wheel:
            // printf("Camera:: mouse wheel %d\n", event.wheel);
            updateRadius(-static_cast<float>(event.wheel) / 240.0f);
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
    }

private:
    float r = 20;
    float theta = 0;
    float phi = 0;
    bool mouseActive = false;
    int mouseX = 0;
    int mouseY = 0;
    float halfPi = 3.14159 / 2;
};


std::unique_ptr<ICamera> createCamera() {
    return std::make_unique<Camera>();
}

};