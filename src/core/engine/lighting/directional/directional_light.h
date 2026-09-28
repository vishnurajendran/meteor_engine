//
// directional_light.h
//
#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H
#include "directional_light_gpu_struct.h"
#include "core/engine/lighting/light_entity.h"

SCRIPT_BIND_CLASS()
class MDirectionalLight : public MLightEntity
{
    DEFINE_SPATIAL_CLASS(MDirectionalLight)

    DECLARE_FIELD(color,        SVector3, SVector3(1.0f, 1.0f, 1.0f))
    DECLARE_FIELD(intensity,    float,    1.0f)
    DECLARE_FIELD(castsShadow,  bool,     true)
    DECLARE_FIELD(smoothShadow, bool,     false)
    // How far in front of the camera directional shadows are drawn, in world
    // units. Larger values cover more of the scene but spread the same shadow
    // map over a bigger area, so shadows get softer / blockier.
    DECLARE_FIELD(shadowDistance, float,  100.0f)

public:
    MDirectionalLight();
    ~MDirectionalLight() override = default;

    SCRIPT_BIND_FUNC()
    void   setColor(const SColor& color)       override;
    SCRIPT_BIND_FUNC()
    SColor getColor() const                     override;
    SCRIPT_BIND_FUNC()
    void   setIntensity(const float& intensity) override;
    SCRIPT_BIND_FUNC()
    float  getIntensity() const                 override;

    SCRIPT_BIND_FUNC()
    bool getCastsShadow()  const { return castsShadow.get(); }
    SCRIPT_BIND_FUNC()
    bool getSmoothShadow() const { return smoothShadow.get(); }
    SCRIPT_BIND_FUNC()
    void setCastsShadow(bool v)  { castsShadow = v; }
    SCRIPT_BIND_FUNC()
    void setSmoothShadow(bool v) { smoothShadow = v; }

    static constexpr float MIN_SHADOW_DISTANCE = 5.0f;
    static constexpr float MAX_SHADOW_DISTANCE = 1000.0f;

    SCRIPT_BIND_FUNC()
    float getShadowDistance() const { return shadowDistance.get(); }
    SCRIPT_BIND_FUNC()
    void  setShadowDistance(float v)
    {
        shadowDistance = glm::clamp(v, MIN_SHADOW_DISTANCE, MAX_SHADOW_DISTANCE);
    }

    SCRIPT_BIND_FUNC()
    void prepareLightRender() override;

    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

private:
    SDirectionalLightData lightData;
    unsigned int bufferId = 0;
    static MDirectionalLight* lightInstance;
};

#endif // DIRECTIONAL_LIGHT_H