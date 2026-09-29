//
// Created by ssj5v on 29-04-2025.
//

#ifndef MDYNAMICLIGHT_H
#define MDYNAMICLIGHT_H
#include "../../../graphics/core/render-pipeline/stages/lighting/dynamic_light_data.h"
#include "core/engine/lighting/light_entity.h"

SCRIPT_BIND_CLASS()
class MDynamicLight : public MLightEntity {
    DEFINE_OBJECT_SUBCLASS(MDynamicLight)
protected:
    SDynamicLightDataStruct lightData;
public:

    SCRIPT_BIND_FUNC()
    [[nodiscard]] float getRange() const { return lightData.range; }
    SCRIPT_BIND_FUNC()
    void setColor(const SColor& color) override;
    SCRIPT_BIND_FUNC()
    SColor getColor() const override;

    SCRIPT_BIND_FUNC()
    void setIntensity(const float& intensity) override;
    SCRIPT_BIND_FUNC()
    float getIntensity() const override;

    SCRIPT_BIND_FUNC()
    void setRange(const float& range);
    void prepareLightRender() override;

    SDynamicLightDataStruct getLightData() const;

    virtual void onUpdate(float deltaTime) override;

    // ---- Shadow support -------------------------------------------------------

    // Whether this light renders a shadow map each frame.
    // Defaults to true. Set false for cheap lights that don't need shadows.
    SCRIPT_BIND_FUNC()
    bool getCastsShadow() const  { return castsShadow; }
    SCRIPT_BIND_FUNC()
    void setCastsShadow(bool v)  { castsShadow = v; }

    // Written by MShadowStage each frame - the slot index in the shadow map array.
    // -1 = no shadow map this frame.
    // Read by MLightSystemManager::prepareDynamicLights() to fill the SSBO.
    int  getShadowIndex() const   { return lightData.shadowIndex; }
    void setShadowIndex(int idx)  { lightData.shadowIndex = idx; }

    SCRIPT_BIND_FUNC()
    bool getSmoothShadow() const  { return lightData.smoothShadow != 0; }
    SCRIPT_BIND_FUNC()
    void setSmoothShadow(bool v)  { lightData.smoothShadow = v ? 1 : 0; }

private:
    static constexpr int EpsilonDist  = 0.001f;
    static constexpr int EpsilonAngle = 0.035f;

    bool        castsShadow     = true;
    SVector3    prevPosition    = {};
    SQuaternion prevOrientation = {};
};

#endif //MDYNAMICLIGHT_H