#pragma once
#include <memory>
#include <utils/app.h>

namespace gpu {



class ICamera {
public:
    virtual ~ICamera() = default;
    virtual void updateRadius(float dr) = 0;
    virtual void updateTheta(float dTheta) = 0;
    virtual void updatePhi(float dPhi) = 0;
    virtual void getCartesian(float& x, float& y, float& z) = 0;
    virtual void mouseEvent(MouseEvent event) = 0;
};

std::unique_ptr<ICamera> createCamera();

}