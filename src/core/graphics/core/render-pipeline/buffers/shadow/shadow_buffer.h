#pragma once
#ifndef SSHADOW_BUFFER_H
#define SSHADOW_BUFFER_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"

// Central shadow data store - one per pipeline.
// MShadowStage writes all shadow maps into this each frame.
// MLightingStage reads them and binds textures to shader units.
class SShadowBuffer : public SDepthBuffer
{
public:
    static constexpr int MAX_SPOT_SHADOWS      = 4;
    static constexpr int MAX_POINT_SHADOWS     = 4;

    // Not resized by resizeAll() - fixed resolution.
    bool isResizeable() const override { return false; }

    // ---- Directional (cascaded) ---------------------------------------------
    // The view up to the light's shadow distance is split into numCascades
    // depth ranges. Each gets its own CASCADE_RESOLUTION map: one layer of
    // cascadeArrayTexture (GL_TEXTURE_2D_ARRAY, depth, compare mode on, so
    // shaders sample it as sampler2DArrayShadow with free bilinear PCF).
    static constexpr int MAX_CASCADES       = 4;   // must match meteor_lighting_utils.glsl
    static constexpr int CASCADE_RESOLUTION = 2048;

    unsigned int cascadeArrayTexture = 0;
    unsigned int cascadeFBO          = 0;   // layer re-attached per cascade
    int          numCascades         = 0;   // 0 = no directional shadow this frame

    glm::mat4 cascadeMatrices[MAX_CASCADES];        // world -> light clip space
    float     cascadeSplits[MAX_CASCADES]     = {}; // far edge of each cascade, view-space distance
    float     cascadeTexelWorld[MAX_CASCADES] = {}; // world units covered by one texel
    glm::mat4 cascadeCameraView = glm::mat4(1.0f);  // camera view the splits were measured in
    float     shadowDistance    = 0.0f;

    // Allocate / free the cascade texture array. Called by MShadowStage::init().
    void initCascades(int resolution);
    void destroyCascades();

    // ---- Spot ---------------------------------------------------------------
    unsigned int spotFBOs[MAX_SPOT_SHADOWS]          = {};
    unsigned int spotDepthTextures[MAX_SPOT_SHADOWS] = {};
    glm::mat4    spotLightSpaceMatrices[MAX_SPOT_SHADOWS]; // for UV projection only
    glm::vec3    spotLightPositions[MAX_SPOT_SHADOWS];     // for linear distance compare
    float        spotFarPlanes[MAX_SPOT_SHADOWS]     = {};
    int          numSpotShadows = 0;

    // ---- Point (cube shadow maps) -------------------------------------------
    unsigned int pointFBOs[MAX_POINT_SHADOWS]            = {}; // reused per-face per-frame
    unsigned int pointCubemapTextures[MAX_POINT_SHADOWS] = {};
    glm::vec3    pointLightPositions[MAX_POINT_SHADOWS];
    float        pointFarPlanes[MAX_POINT_SHADOWS]       = {};
    int          numPointShadows = 0;

    // Allocate / free spot and point shadow GL objects.
    // Called by MShadowStage::init() - separate from the directional SDepthBuffer.
    void initDynamicShadows(int shadowMapRes);
    void destroyDynamicShadows();
};

#endif