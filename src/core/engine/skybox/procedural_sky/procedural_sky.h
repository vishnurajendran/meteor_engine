//
// procedural_sky.h
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
    DEFINE_CLASS(MProceduralSkyboxEntity)

    // ── Serialized sky parameters ─────────────────────────────────────────────
    DECLARE_FIELD(elevationDeg,        float,    55.0f)
    DECLARE_FIELD(azimuthDeg,          float,    30.0f)
    DECLARE_FIELD(cycleSpeed,          float,    0.0f)
    DECLARE_FIELD(sunSize,             float,    0.04f)
    DECLARE_FIELD(sunSizeConvergence,  float,    5.0f)
    DECLARE_FIELD(atmosphereThickness, float,    1.0f)
    DECLARE_FIELD(skyTint,             SVector3, SVector3(0.5f))
    DECLARE_FIELD(groundColor,         SVector3, SVector3(0.22f, 0.20f, 0.18f))
    DECLARE_FIELD(exposure,            float,    1.3f)

public:
    MProceduralSkyboxEntity();
    ~MProceduralSkyboxEntity() override;

    // ── IMeteorDrawable ───────────────────────────────────────────────────────
    void submitRenderItem(IRenderItemCollector* collector) override;
    bool canDraw() override { return getEnabled(); }

    void onExit()                  override;
    void onUpdate(float deltaTime) override;
    void onDrawGizmo(SVector2 res) override;

    // ── Sun direction ─────────────────────────────────────────────────────────
    void setSunAngles(float elevationDeg, float azimuthDeg);
    void setSunDirection(const SVector3& dir);
    [[nodiscard]] SVector3 getSunDirection() const;

    // ── Parameter API — mirror the DECLARE_FIELDs for external code ──────────
    void  setDayNightCycleSpeed(float cyclesPerSec) { cycleSpeed = cyclesPerSec; }
    float getDayNightCycleSpeed() const             { return cycleSpeed.get(); }

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

protected:
    // Push all field values into the draw call after deserialization
    void onDeserialise(const pugi::xml_node& node) override;

private:
    void rebuildSunDirection();
    void pushFieldsToDrawCall();                // syncs all DECLARE_FIELDs → drawCall
    void updateAmbientLight(MAmbientLightEntity* light) const;
    [[nodiscard]] MDirectionalLight*   findDirectionalLightChild() const;
    [[nodiscard]] MAmbientLightEntity* findAmbientLightChild()     const;

    MProceduralSkyboxDrawCall* drawCall = nullptr;
};

#endif // PROCEDURAL_SKY_H