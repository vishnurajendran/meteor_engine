//
// gpu_skinning_backend.h
//
// Compute-shader skinning. One dispatch per sub-mesh instance (batching all
// instances into one dispatch is a later optimisation), then a single
// glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT) before any stage draws.
//
// The compute shader is an engine asset:
//   meteor_assets/engine_assets/shaders/internal/skinning.mesl  (#COMPUTE_PASS)
// It hot-reloads like any other shader; a broken edit keeps the last good
// program, and a missing file makes the subsystem fall back to CPU skinning.
//
// SSBO binding points are picked at runtime from the top of the range, never
// LIGHT_INDEX_DYNAMIC: the lighting system binds its buffer there once and
// expects it to stay.
//
#pragma once
#ifndef GPU_SKINNING_BACKEND_H
#define GPU_SKINNING_BACKEND_H

#include "skinning_backend.h"

class MComputeShader;

class MGpuSkinningBackend : public ISkinningBackend
{
public:
    ~MGpuSkinningBackend() override { cleanup(); }

    bool init() override;
    void cleanup() override;
    void skin(const std::vector<SMatrix4>& matrices, const std::vector<SSkinningJob>& jobs) override;
    [[nodiscard]] const char* getName() const override { return "GPU (compute)"; }

    // GPU time of the last frame's dispatches, ms (a frame late), or -1.
    [[nodiscard]] float getLastGpuTimeMs() const { return lastGpuTimeMs; }

private:
    void readTimerQuery();
    // Resolves the shader asset and, if its MComputeShader changed (first use
    // or a hot reload), compiles it and sets its buffer block bindings. False
    // when there is no usable shader.
    bool refreshShader();

    MComputeShader* activeShader = nullptr;   // last good one - dispatched with; owned by the asset
    MComputeShader* seenShader   = nullptr;   // latest from the asset, even if it failed to compile
    bool            warnedMissingShader = false;
    unsigned int boneBuffer     = 0;
    size_t       boneBufferSize = 0;   // bytes allocated

    // Binding points for BindPose, Offsets, Influences, Bones, Out.
    unsigned int bindings[5] = {};

    unsigned int timerQueries[2] = {};
    int          timerFrame      = 0;
    bool         timerPending[2] = {};
    float        lastGpuTimeMs   = -1.0f;
};

#endif // GPU_SKINNING_BACKEND_H
