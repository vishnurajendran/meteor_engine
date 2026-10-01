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

    // Serialized spot angle — stored in RADIANS, same unit as lightData.angle,
    // so existing behaviour is unchanged. The public API still speaks degrees.
    // Default mirrors SDynamicLightDataStruct::angle.
    DECLARE_FIELD(spotAngle, float, 45.0f)

public:
    MSpotLight();
    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

    SCRIPT_BIND_FUNC()
    [[nodiscard]] float getSpotAngle() const;   // returns degrees
    SCRIPT_BIND_FUNC()
    void setSpotAngle(float angleDeg);

protected:
    void onDeserialise(const pugi::xml_node& node) override;

private:
    void drawSpotLightGizmo();
};

#endif // SPOT_LIGHT_H