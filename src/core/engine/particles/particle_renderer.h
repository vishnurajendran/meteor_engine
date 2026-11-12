//
// particle_renderer.h
//
// IParticleRenderer — how an emitter's particles are drawn. One per emitter.
//
// Renderers are IMeteorDrawables: they register with the render queue
// themselves and, every frame, submit one instanced SRenderItem per piece
// of geometry (one for a sprite quad, one per sub-mesh for a mesh). The
// transparent stage draws it with one instance per particle.
//
//   prepare()  — sorts, packs and uploads the alive particles into the
//                instance buffer. Called from submitRenderItem, so it sees
//                this frame's camera (sorting) and runs once per frame.
//   cleanup()  — unregisters from the render queue and frees GL objects.
//
// MParticleRendererBase does all of that; concrete renderers only provide
// their geometry (syncGeometry) and whether it faces the camera.
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_RENDERER_H
#define METEOR_ENGINE_PARTICLE_RENDERER_H

#include <span>
#include <vector>

#include "core/engine/assetmanagement/asset/asset_ref_handle.h"
#include "core/engine/assetmanagement/asset/field_asset_ref_types.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/render-pipeline/interfaces/drawable_interface.h"
#include "core/object/object.h"
#include "core/utils/aabb.h"
#include "core/utils/field_engine_types.h"
#include "particle_data.h"
#include "particle_settings.h"

class MMaterial;
class MParticleEmitter;
class MShader;

class IParticleRenderer : public IParticleSettings
{
public:
    ~IParticleRenderer() override = default;

    // The emitter whose particles this draws. Set once by the emitter right
    // after creating the renderer.
    virtual void setEmitter(MParticleEmitter* emitter) = 0;

    // Sort, pack and upload — see the top of this file.
    virtual void prepare(std::span<const SParticleData> particles) = 0;

    // Stops drawing and frees GPU resources. Safe to call twice.
    virtual void cleanup() = 0;
};

// One particle as the GPU sees it — per-instance vertex attributes 3-7 in
// particle_shader.glsl. Packed from SParticleData every frame: only what the
// shader reads, in draw (sorted) order. 68 bytes, vs ~108 for SParticleData.
struct SParticleInstance
{
    SVector3 position;   // location 3
    SVector4 rotation;   // location 4 — quaternion x y z w
    SVector3 size;       // location 5
    SVector4 color;      // location 6 — sRGB + alpha
    SVector3 velocity;   // location 7 — simulation space, for velocity alignment
};
static_assert(sizeof(SParticleInstance) == 68, "SParticleInstance must be tightly packed — it is a vertex buffer layout");

// How each particle is turned (MParticleRendererBase::alignment).
//   View     — sprites face the camera. Meshes: particle rotation only.
//   Velocity — sprites: up axis along the movement, still turned toward the
//              camera. Meshes: +Z points along the movement.
//   ZAxis    — geometry faces simulation-space +Z (emitter Z in Local, world Z
//              in Global), turned by the particle rotation.
enum class EParticleAlignment : int { View = 0, Velocity = 1, ZAxis = 2 };

class MParticleRendererBase : public MObject, public IParticleRenderer, public IMeteorDrawable
{
    DEFINE_OBJECT_SUBCLASS(MParticleRendererBase)

    DECLARE_FIELD(texture,        TAssetRef<MTextureAsset>, {})
    DECLARE_FIELD(blendMode,      int,  0)       // 0 Alpha — 1 Additive
    DECLARE_FIELD(lit,            bool, false)
    DECLARE_FIELD(sortByDistance, bool, true)    // back to front — alpha blend only
    DECLARE_FIELD(alignment,      int,  0)       // EParticleAlignment — int so the generic inspector shows a combo

public:
    MParticleRendererBase();
    ~MParticleRendererBase() override;

    // IParticleRenderer
    void setEmitter(MParticleEmitter* emitter) override { owner = emitter; }
    void prepare(std::span<const SParticleData> particles) override;
    void cleanup() override;

    // IMeteorDrawable
    void submitRenderItem(IRenderItemCollector* collector) override;
    bool canDraw() override;

    [[nodiscard]] SParticleFieldMeta getFieldMeta(const std::string& fieldName) const override;

    [[nodiscard]] const AABB& getBounds() const { return worldBounds; }

protected:
    struct SGeometry
    {
        unsigned int vao         = 0;   // owned by this renderer — has the instance attributes
        unsigned int ebo         = 0;   // 0 → glDrawArrays
        int          indexCount  = 0;
        int          vertexCount = 0;
    };

    // Make `geometry` current (create on first call, rebuild when the
    // source changed). Use createInstancedVAO for each VAO. Return false
    // when there is nothing to draw (e.g. no mesh assigned).
    virtual bool syncGeometry() = 0;

    // Free what syncGeometry created. The base calls it from cleanup().
    // Default: deletes every VAO in `geometry` and clears it.
    virtual void destroyGeometry();

    // True: sprite quads (camera-facing by default, soft dot without a
    // texture). False: meshes.
    [[nodiscard]] virtual bool useBillboards() const = 0;

    // `alignment` + geometry kind → the shader's _alignment mode:
    //   0 billboard, 1 velocity billboard, 2 rotation only, 3 mesh along velocity.
    [[nodiscard]] int getShaderAlignmentMode() const;

    // A VAO reading SVertex data (locations 0-2) from vertexBuffer and one
    // SParticleInstance per instance (locations 3-6) from instanceVBO. The
    // VAO also captures elementBuffer when non-zero.
    [[nodiscard]] unsigned int createInstancedVAO(unsigned int vertexBuffer, unsigned int elementBuffer);

    std::vector<SGeometry> geometry;
    MParticleEmitter*      owner = nullptr;

private:
    bool syncMaterial();
    void ensureInstanceBuffer();
    void uploadInstances();

    MMaterial* material       = nullptr;   // owned
    MShader*   materialShader = nullptr;   // owned by the shader asset

    unsigned int instanceVBO      = 0;
    size_t       instanceCapacity = 0;      // in instances, not bytes
    bool         registered       = false;  // in the render queue

    // Scratch buffers, reused every frame — no per-frame allocation once warm.
    std::vector<SParticleInstance> instances;
    std::vector<uint32_t>          drawOrder;
    std::vector<float>             sortKeys;

    AABB worldBounds = { SVector3(0.0f), SVector3(0.0f) };
};

#endif // METEOR_ENGINE_PARTICLE_RENDERER_H
