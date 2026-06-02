//
// ambient_light.h
//
#ifndef AMBIENT_LIGHTS_H
#define AMBIENT_LIGHTS_H

#include "ambient_light_gpu_struct.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/lighting/light_entity.h"
#include "GL/glew.h"

class MAmbientLightEntity : public MLightEntity
{
    DEFINE_SPATIAL_CLASS(MAmbientLightEntity)

    // Serialized - color as RGB, intensity as float.
    // prepareLightRender() pushes these into the GPU struct each frame.
    DECLARE_FIELD(color,     SVector3, SVector3(1.0f, 1.0f, 1.0f))
    DECLARE_FIELD(intensity, float,    1.0f)

public:
    MAmbientLightEntity();
    ~MAmbientLightEntity() override = default;

    void prepareLightRender() override;

    void   setColor(const SColor& color)       override;
    SColor getColor() const                     override;
    void   setIntensity(const float& intensity) override;
    float  getIntensity() const                 override;

    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

private:
    unsigned int ambientLightDataBufferId = 0;
    SAmbientLightData ambientLightData;             // GPU-side struct; synced in prepareLightRender()
    static MAmbientLightEntity* ambientLightInstance;
};

#endif // AMBIENT_LIGHTS_H