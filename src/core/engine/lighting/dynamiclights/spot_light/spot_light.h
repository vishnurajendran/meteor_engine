//
// spot_light.h
//
#ifndef SPOT_LIGHT_H
#define SPOT_LIGHT_H
#include "core/engine/lighting/dynamiclights/dynamic_light.h"

SCRIPT_BIND_CLASS()
class MSpotLight : public MDynamicLight
{
    DEFINE_SPATIAL_CLASS(MSpotLight)
    // spotAngle lives in lightData.angle (stored in radians, exposed in degrees via API).
    // If MDynamicLight does not DECLARE_FIELD it, add: DECLARE_FIELD(spotAngleDeg, float, 30.0f)
    // and sync in onDeserialise / prepareLightRender.

public:
    MSpotLight();
    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

    SCRIPT_BIND_FUNC()
    [[nodiscard]] float getSpotAngle() const;   // returns degrees
    SCRIPT_BIND_FUNC()
    void setSpotAngle(float angleDeg);

private:
    void drawSpotLightGizmo();
};

#endif // SPOT_LIGHT_H