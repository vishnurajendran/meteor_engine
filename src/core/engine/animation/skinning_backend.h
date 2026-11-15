//
// skinning_backend.h
//
// ISkinningBackend - turns bind-pose vertices plus this frame's skinning
// matrices into each entity's skinned vertex buffer (SVertex layout).
//
//   MGpuSkinningBackend  compute shader, once per frame (the default)
//   MCpuSkinningBackend  MJobSystem on the CPU, then an upload - a reference
//                        for when the GPU output looks wrong
//
// Both write the same output buffer, which every render pass then draws.
//
#pragma once
#ifndef SKINNING_BACKEND_H
#define SKINNING_BACKEND_H

#include <cstdint>
#include <vector>

#include "core/utils/glmhelper.h"

class MSkeletalMesh;

enum class ESkinningBackend : int
{
    GPU = 0,
    CPU = 1,
};

struct SSkinningJob
{
    const MSkeletalMesh* mesh         = nullptr;
    unsigned int         outputBuffer = 0;   // vertexCount * sizeof(SVertex) bytes
    uint32_t             boneOffset   = 0;   // first skinning matrix of the instance
};

class ISkinningBackend
{
public:
    virtual ~ISkinningBackend() = default;

    // Main thread, GL context current. False if the backend can't run here
    // (the subsystem then falls back to the other one).
    virtual bool init() = 0;
    virtual void cleanup() = 0;

    // Every instance's skinning matrices, back to back. Called once a frame
    // before skin(); `matrices` stays valid until skin() returns.
    virtual void skin(const std::vector<SMatrix4>& matrices, const std::vector<SSkinningJob>& jobs) = 0;

    [[nodiscard]] virtual const char* getName() const = 0;
};

#endif // SKINNING_BACKEND_H
