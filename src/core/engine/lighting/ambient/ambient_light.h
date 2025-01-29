//
// Created by ssj5v on 24-01-2025.
//

#ifndef AMBIENT_LIGHTS_H
#define AMBIENT_LIGHTS_H
#include "ambient_light_gpu_struct.h"
#include "GL/glew.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/lighting/light_entity.h"


class MAmbientLightEntity : public MLightEntity {
public:
    MAmbientLightEntity();
    ~MAmbientLightEntity() override = default;

    void prepareLightRender() override;

    void setColor(const SColor& color) override;
    [[nodiscard]] SColor getColor() const override;

    void setIntensity(const float& intensity) override;
    [[nodiscard]] float getIntensity() const override;

    virtual void onExit() override;
private:
    GLuint ambientLightDataBufferId;
    SAmbientLightData ambientLightData;
    static MAmbientLightEntity* ambientLightInstance;
};



#endif //AMBIENT_LIGHTS_H
