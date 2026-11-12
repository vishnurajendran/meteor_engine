//
// Mesh particle renderer — one mesh per particle, rotated by the particle.
//

#include <GL/glew.h>
#include "mesh_particle_renderer.h"

#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/particles/particle_registry.h"

REGISTER_PARTICLE_RENDERER(MMeshParticleRenderer, "Mesh")

MMeshParticleRenderer::~MMeshParticleRenderer()
{
    cleanup();
}

SParticleFieldMeta MMeshParticleRenderer::getFieldMeta(const std::string& fieldName) const
{
    if (fieldName == "mesh")
    {
        SParticleFieldMeta meta;
        meta.tooltip = "Drawn once per particle, scaled by the particle size. Every sub-mesh is drawn.";
        return meta;
    }
    return MParticleRendererBase::getFieldMeta(fieldName);
}

bool MMeshParticleRenderer::syncGeometry()
{
    MStaticMeshAsset* asset = mesh.get().resolve();
    if (!asset)
    {
        if (!geometry.empty()) { destroyGeometry(); builtFrom.clear(); }
        return false;
    }

    // Cheap check every frame: same sub-meshes as last time?
    const std::vector<MStaticMesh*> meshes = asset->getMeshes();
    bool same = meshes.size() == builtFrom.size();
    for (size_t i = 0; same && i < meshes.size(); ++i)
        same = meshes[i] && meshes[i]->getVBO() == builtFrom[i];
    if (same) return !geometry.empty();

    // Rebuild. The mesh owns its VBO / EBO — we only make VAOs that read them.
    destroyGeometry();
    builtFrom.clear();
    for (const MStaticMesh* m : meshes)
    {
        builtFrom.push_back(m ? m->getVBO() : 0);
        if (!m || m->getVBO() == 0) continue;

        SGeometry g;
        g.vao         = createInstancedVAO(m->getVBO(), m->getEBO());
        g.ebo         = m->getEBO();
        g.indexCount  = m->getIndexCount();
        g.vertexCount = m->getVertexCount();
        geometry.push_back(g);
    }
    return !geometry.empty();
}
