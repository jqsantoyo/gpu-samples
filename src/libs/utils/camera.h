#pragma once
#include <utils/utils.h>
#include <utils/app.h>
#include <memory>

namespace gpu {



class ICamera {
public:
    virtual ~ICamera() = default;
    virtual void updateRadius(float dr) = 0;
    virtual void updateTheta(float dTheta) = 0;
    virtual void updatePhi(float dPhi) = 0;
    virtual vec3 getCartesian() = 0;
    virtual void mouseEvent(MouseEvent event) = 0;
};

std::unique_ptr<ICamera> createCamera();

}