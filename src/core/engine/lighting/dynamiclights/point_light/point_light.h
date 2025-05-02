//
// Created by ssj5v on 29-04-2025.
//

#ifndef POINT_LIGHT_H
#define POINT_LIGHT_H
#include "core/engine/lighting/dynamiclights/dynamic_light.h"
#include "core/engine/lighting/light_entity.h"

class MPointLight : public MDynamicLight {
public:
    MPointLight();
    void onExit() override;
    void onDrawGizmo() override;
};

#endif //POINT_LIGHT_H
