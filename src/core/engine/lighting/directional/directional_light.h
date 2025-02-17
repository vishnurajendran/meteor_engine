//
// Created by ssj5v on 26-01-2025.
//

#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H
#include <GL/glew.h>
#include "directional_light_gpu_struct.h"
#include "core/engine/lighting/light_entity.h"

class MDirectionalLight : public MLightEntity {
public:
    void setColor(const SColor& color) override;
    SColor getColor() const override;
    void setIntensity(const float& intensity) override;
    float getIntensity() const override;

    void prepareLightRender() override;

    MDirectionalLight();
    ~MDirectionalLight() override = default;
    void onExit() override;
private:
    SDirectionalLightData lightData;
    GLuint bufferId=0;
    static MDirectionalLight* lightInstance;
};

#endif //DIRECTIONAL_LIGHT_H
