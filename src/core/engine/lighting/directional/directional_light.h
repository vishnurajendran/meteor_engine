//
// directional_light.h
//
#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H
#include <GL/glew.h>
#include "directional_light_gpu_struct.h"
#include "core/engine/lighting/light_entity.h"

class MDirectionalLight : public MLightEntity
{
    DEFINE_CLASS(MDirectionalLight)

    DECLARE_FIELD(color,     SVector3, SVector3(1.0f, 1.0f, 1.0f))
    DECLARE_FIELD(intensity, float,    1.0f)

public:
    MDirectionalLight();
    ~MDirectionalLight() override = default;

    void   setColor(const SColor& color)       override;
    SColor getColor() const                     override;
    void   setIntensity(const float& intensity) override;
    float  getIntensity() const                 override;

    void prepareLightRender() override;
    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

private:
    SDirectionalLightData lightData;        // GPU struct; synced in prepareLightRender()
    GLuint bufferId = 0;
    static MDirectionalLight* lightInstance;
};

#endif // DIRECTIONAL_LIGHT_H