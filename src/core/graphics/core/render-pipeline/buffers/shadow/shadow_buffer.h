#pragma once
#ifndef SSHADOW_BUFFER_H
#define SSHADOW_BUFFER_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"

// Central shadow data store — one per pipeline.
// MShadowStage writes all shadow maps into this each frame.
// MLightingStage reads them and binds textures to shader units.
class SShadowBuffer : public SDepthBuffer
{
public:
    static constexpr int SHADOW_MAP_RESOLUTION = 2048;
    static constexpr int MAX_SPOT_SHADOWS      = 4;
    static constexpr int MAX_POINT_SHADOWS     = 4;

    // Not resized by resizeAll() — fixed resolution.
    bool isResizeable() const override { return false; }

    // ---- Directional --------------------------------------------------------
    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);

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
    // Called by MShadowStage::init() — separate from the directional SDepthBuffer.
    void initDynamicShadows(int shadowMapRes);
    void destroyDynamicShadows();
};

#endif