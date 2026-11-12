//
// Mesh particle renderer — one mesh per particle, rotated by the particle.
//
#pragma once
#ifndef METEOR_ENGINE_MESH_PARTICLE_RENDERER_H
#define METEOR_ENGINE_MESH_PARTICLE_RENDERER_H

#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/particles/particle_renderer.h"

// draws meshes instanced using data
class MMeshParticleRenderer : public MParticleRendererBase
{
    DEFINE_OBJECT_SUBCLASS(MMeshParticleRenderer)

    DECLARE_FIELD(mesh, TAssetRef<MStaticMeshAsset>, {})

public:
    MMeshParticleRenderer() = default;
    ~MMeshParticleRenderer() override;

    [[nodiscard]] SParticleFieldMeta getFieldMeta(const std::string& fieldName) const override;

protected:
    bool syncGeometry() override;
    [[nodiscard]] bool useBillboards() const override { return false; }

private:
    // VBO of each sub-mesh the VAOs were built from. When the asset is
    // swapped or hot-reloaded the sub-meshes are new objects with new
    // buffers, so a mismatch means "rebuild".
    std::vector<unsigned int> builtFrom;
};

#endif // METEOR_ENGINE_MESH_PARTICLE_RENDERER_H
