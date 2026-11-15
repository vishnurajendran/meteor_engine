//
// cpu_skinning_backend.h
//
// Reference skinning on the CPU through MJobSystem: the same maths as the
// compute shader, written into the same output buffer with one upload per
// sub-mesh. Slow for crowds (it uploads every skinned vertex every frame);
// meant for checking the GPU path and for contexts without compute shaders.
//
#pragma once
#ifndef CPU_SKINNING_BACKEND_H
#define CPU_SKINNING_BACKEND_H

#include "core/engine/3d/staticmesh/staticmesh.h"
#include "skinning_backend.h"

class MCpuSkinningBackend : public ISkinningBackend
{
public:
    bool init() override { return true; }
    void cleanup() override { scratch.clear(); scratch.shrink_to_fit(); }
    void skin(const std::vector<SMatrix4>& matrices, const std::vector<SSkinningJob>& jobs) override;
    [[nodiscard]] const char* getName() const override { return "CPU (jobs)"; }

private:
    std::vector<std::vector<SVertex>> scratch;   // one per job, reused
};

#endif // CPU_SKINNING_BACKEND_H
