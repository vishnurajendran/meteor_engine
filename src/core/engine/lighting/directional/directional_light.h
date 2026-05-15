//
// directional_light.h
//
#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H
#include "directional_light_gpu_struct.h"
#include "core/engine/lighting/light_entity.h"

class MDirectionalLight : public MLightEntity
{
    DEFINE_SPATIAL_CLASS(MDirectionalLight)

    DECLARE_FIELD(color,        SVector3, SVector3(1.0f, 1.0f, 1.0f))
    DECLARE_FIELD(intensity,    float,    1.0f)
    DECLARE_FIELD(castsShadow,  bool,     true)
    DECLARE_FIELD(smoothShadow, bool,     false)

public:
    MDirectionalLight();
    ~MDirectionalLight() override = default;

    void   setColor(const SColor& color)       override;
    SColor getColor() const                     override;
    void   setIntensity(const float& intensity) override;
    float  getIntensity() const                 override;

    bool getCastsShadow()  const { return castsShadow.get(); }
    bool getSmoothShadow() const { return smoothShadow.get(); }
    void setCastsShadow(bool v)  { castsShadow = v; }
    void setSmoothShadow(bool v) { smoothShadow = v; }

    void prepareLightRender() override;
    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

private:
    SDirectionalLightData lightData;
    unsigned int bufferId = 0;
    static MDirectionalLight* lightInstance;
};

#endif // DIRECTIONAL_LIGHT_H