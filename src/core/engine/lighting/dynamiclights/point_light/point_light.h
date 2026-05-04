//
// point_light.h
//
#ifndef POINT_LIGHT_H
#define POINT_LIGHT_H
#include "core/engine/lighting/dynamiclights/dynamic_light.h"
#include "core/engine/lighting/light_entity.h"

class MPointLight : public MDynamicLight
{
    DEFINE_CLASS(MPointLight)
    // Note: range and attenuation live in MDynamicLight's lightData.
    // Add DECLARE_FIELDs here for any MPointLight-specific serializable properties.

public:
    MPointLight();
    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;
};

#endif // POINT_LIGHT_H