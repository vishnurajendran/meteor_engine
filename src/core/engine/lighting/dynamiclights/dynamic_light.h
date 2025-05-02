//
// Created by ssj5v on 29-04-2025.
//

#ifndef MDYNAMICLIGHT_H
#define MDYNAMICLIGHT_H
#include "core/engine/lighting/light_entity.h"
#include "dynamic_light_data.h"


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
private:

    static constexpr int EpsilonDist = 0.001f;
    static constexpr int EpsilonAngle = 0.035f;

    SVector3 prevPosition = {};
    SQuaternion prevOrientation = {};
};



#endif //MDYNAMICLIGHT_H
