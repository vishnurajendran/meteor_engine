//
// MProceduralSkyboxDrawCall
// Self-contained draw call — fetches camera matrices internally inside draw(),
// mirroring how MSkyboxDrawCall works.
//

#pragma once
#ifndef PROCEDURAL_SKYBOX_DRAW_CALL_H
#define PROCEDURAL_SKYBOX_DRAW_CALL_H

#include "core/utils/glmhelper.h"

class MShader;

class MProceduralSkyboxDrawCall
{
public:
    explicit MProceduralSkyboxDrawCall(MShader* shader);
    ~MProceduralSkyboxDrawCall();

    // Called by MProceduralSkyboxStage — mirrors MSkyboxDrawCall interface.
    void setTargetResolution(const SVector2& res) { resolution = res; }
    void draw();

    // ---- Sky parameters ----------------------------------------------------
    void setSunDirection       (const SVector3& dir) { sunDirection        = glm::normalize(dir); }
    void setSunSize            (float v)             { sunSize             = v; }
    void setSunSizeConvergence (float v)             { sunSizeConvergence  = v; }
    void setAtmosphereThickness(float v)             { atmosphereThickness = v; }
    void setSkyTint            (const SVector3& v)   { skyTint             = v; }
    void setGroundColor        (const SVector3& v)   { groundColor         = v; }
    void setExposure           (float v)             { exposure            = v; }

    [[nodiscard]] SVector3 getSunDirection()        const { return sunDirection; }
    [[nodiscard]] float    getSunSize()             const { return sunSize; }
    [[nodiscard]] float    getSunSizeConvergence()  const { return sunSizeConvergence; }
    [[nodiscard]] float    getAtmosphereThickness() const { return atmosphereThickness; }
    [[nodiscard]] SVector3 getSkyTint()             const { return skyTint; }
    [[nodiscard]] SVector3 getGroundColor()         const { return groundColor; }
    [[nodiscard]] float    getExposure()            const { return exposure; }

private:
    void ensureCubeReady();
    void uploadUniforms(const SMatrix4& proj, const SMatrix4& rotView) const;

    MShader*     shader     = nullptr;
    unsigned int cubeVAO    = 0;
    unsigned int cubeVBO    = 0;
    SVector2     resolution = SVector2(1280, 720);

    SVector3 sunDirection        = SVector3(0.0f, 1.0f, 0.0f);
    float    sunSize             = 0.04f;
    float    sunSizeConvergence  = 5.0f;
    float    atmosphereThickness = 1.0f;
    SVector3 skyTint             = SVector3(0.5f, 0.5f, 0.5f);
    SVector3 groundColor         = SVector3(0.22f, 0.20f, 0.18f);
    float    exposure            = 1.3f;
};

#endif // PROCEDURAL_SKYBOX_DRAW_CALL_H