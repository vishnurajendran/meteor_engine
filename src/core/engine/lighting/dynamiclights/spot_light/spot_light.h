//
// Created by ssj5v on 29-04-2025.
//

#ifndef SPOT_LIGHT_H
#define SPOT_LIGHT_H
#include "core/engine/lighting/dynamiclights/dynamic_light.h"


class MSpotLight : public MDynamicLight {
private:
    void drawSpotLightGizmo();
public:
    MSpotLight();

    void onExit() override;
    void onDrawGizmo() override;

    [[nodiscard]] float getSpotAngle() const;
    void setSpotAngle(float angle);
};



#endif //SPOT_LIGHT_H
