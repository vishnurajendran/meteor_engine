//
// MProceduralSkyboxEntity
//

#pragma once
#ifndef PROCEDURAL_SKY_H
#define PROCEDURAL_SKY_H

#include "core/engine/entities/spatial/spatial.h"
#include "core/graphics/core/render-pipeline/interfaces/drawable_interface.h"
#include "core/utils/glmhelper.h"

class MProceduralSkyboxDrawCall;
class MDirectionalLight;
class MAmbientLightEntity;

class MProceduralSkyboxEntity : public MSpatialEntity, public IMeteorDrawable
{
public:
    MProceduralSkyboxEntity();
    ~MProceduralSkyboxEntity() override;

    SString typeName() const override { return STR("procedural_skybox"); }

    void submitRenderItem(IRenderItemCollector* collector) override;
    bool canDraw() override { return getEnabled(); }

    void onExit()                  override;
    void onUpdate(float deltaTime) override;
    void onDrawGizmo(SVector2 res) override;

    // ---- Sun direction -----------------------------------------------------
    void setSunAngles(float elevationDeg, float azimuthDeg);
    void setSunDirection(const SVector3& dir);
    [[nodiscard]] SVector3 getSunDirection() const;

    // ---- Day/night cycle ---------------------------------------------------
    // Speed in CYCLES PER SECOND (1 cycle = 360 degree pass of the sun).
    // 0 = static. Ignored when a directional light child drives the direction.
    void  setDayNightCycleSpeed(float cyclesPerSec) { cycleSpeed = cyclesPerSec; }
    float getDayNightCycleSpeed() const             { return cycleSpeed; }

    // ---- Sky parameters ----------------------------------------------------
    void setSunSize            (float v);
    void setSunSizeConvergence (float v);
    void setAtmosphereThickness(float v);
    void setSkyTint            (const SVector3& v);
    void setGroundColor        (const SVector3& v);
    void setExposure           (float v);

    [[nodiscard]] float    getSunSize()             const;
    [[nodiscard]] float    getSunSizeConvergence()  const;
    [[nodiscard]] float    getAtmosphereThickness() const;
    [[nodiscard]] SVector3 getSkyTint()             const;
    [[nodiscard]] SVector3 getGroundColor()         const;
    [[nodiscard]] float    getExposure()            const;

private:
    void rebuildSunDirection();

    [[nodiscard]] MDirectionalLight*  findDirectionalLightChild() const;
    [[nodiscard]] MAmbientLightEntity* findAmbientLightChild()    const;

    // Computes an ambient colour + intensity from the current sun elevation.
    // Used to drive a child MAmbientLightEntity each frame.
    void updateAmbientLight(MAmbientLightEntity* light) const;

    MProceduralSkyboxDrawCall* drawCall = nullptr;

    float elevationDeg = 55.0f;
    float azimuthDeg   = 30.0f;
    float cycleSpeed   = 0.0f;  // cycles per second
};

#endif // PROCEDURAL_SKY_H