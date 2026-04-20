//
// Created by ssj5v on 29-04-2025.
//

#ifndef MDYNAMICLIGHT_H
#define MDYNAMICLIGHT_H
#include "../../../graphics/core/render-pipeline/stages/lighting/dynamic_light_data.h"
#include "core/engine/lighting/light_entity.h"


class MDynamicLight : public MLightEntity {
protected:
    SDynamicLightDataStruct lightData;
public:
    [[nodiscard]] float getRange() const { return lightData.range; }
    void setColor(const SColor& color) override;
    SColor getColor() const override;

    void setIntensity(const float& intensity) override;
    float getIntensity() const override;

    void setRange(const float& range);
    void prepareLightRender() override;

    SDynamicLightDataStruct getLightData() const;

    virtual void onUpdate(float deltaTime) override;

    // ---- Shadow support -------------------------------------------------------

    // Whether this light renders a shadow map each frame.
    // Defaults to true. Set false for cheap lights that don't need shadows.
    bool getCastsShadow() const  { return castsShadow; }
    void setCastsShadow(bool v)  { castsShadow = v; }

    // Written by MShadowStage each frame — the slot index in the shadow map array.
    // -1 = no shadow map this frame.
    // Read by MLightSystemManager::prepareDynamicLights() to fill the SSBO.
    int  getShadowIndex() const   { return lightData.shadowIndex; }
    void setShadowIndex(int idx)  { lightData.shadowIndex = idx; }

    bool getSmoothShadow() const  { return lightData.smoothShadow != 0; }
    void setSmoothShadow(bool v)  { lightData.smoothShadow = v ? 1 : 0; }

private:
    static constexpr int EpsilonDist  = 0.001f;
    static constexpr int EpsilonAngle = 0.035f;

    bool        castsShadow     = true;
    SVector3    prevPosition    = {};
    SQuaternion prevOrientation = {};
};

#endif //MDYNAMICLIGHT_H