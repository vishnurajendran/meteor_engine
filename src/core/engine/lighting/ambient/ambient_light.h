//
// ambient_light.h
//
#ifndef AMBIENT_LIGHTS_H
#define AMBIENT_LIGHTS_H

#include "GL/glew.h"
#include "ambient_light_gpu_struct.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/lighting/light_entity.h"


SCRIPT_BIND_CLASS()
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

    SCRIPT_BIND_FUNC()
    void   setColor(const SColor& color)       override;
    SCRIPT_BIND_FUNC()
    SColor getColor() const                     override;
    SCRIPT_BIND_FUNC()
    void   setIntensity(const float& intensity) override;
    SCRIPT_BIND_FUNC()
    float  getIntensity() const                 override;

    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

private:
    unsigned int ambientLightDataBufferId = 0;
    SAmbientLightData ambientLightData;             // GPU-side struct; synced in prepareLightRender()
    static MAmbientLightEntity* ambientLightInstance;
};

#endif // AMBIENT_LIGHTS_H