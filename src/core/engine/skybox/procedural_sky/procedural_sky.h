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

    void setSunAngles(float elevationDeg, float azimuthDeg);
    void setSunDirection(const SVector3& dir);
    [[nodiscard]] SVector3 getSunDirection() const;

    void  setDayNightCycleSpeed(float degPerSec) { cycleSpeed = degPerSec; }
    float getDayNightCycleSpeed() const          { return cycleSpeed; }

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
    MProceduralSkyboxDrawCall* drawCall = nullptr;
    float elevationDeg = 45.0f;
    float azimuthDeg   = 0.0f;
    float cycleSpeed   = 0.0f;
};

#endif // PROCEDURAL_SKY_H