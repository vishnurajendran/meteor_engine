//
// particle_renderer.cpp
//

#include <GL/glew.h>
#include "particle_renderer.h"

#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <numeric>

#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/camera/camera_spatial_entity.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/render-pipeline/interfaces/render_item_collector.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager_interface.h"
#include "core/graphics/core/render-pipeline/render_queue.h"
#include "core/graphics/core/render-pipeline/stages/render_stage.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/memory/mnew.h"
#include "particle_emitter.h"

static constexpr const char* PARTICLE_SHADER = "meteor_assets/engine_assets/shaders/default/particle.mesl";

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

MParticleRendererBase::MParticleRendererBase()
{
    if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
    {
        renderer->getRenderQueue().addToSubmitables(this);
        registered = true;
    }
}

MParticleRendererBase::~MParticleRendererBase()
{
    // Derived destructors have already run, so a derived destroyGeometry()
    // override is no longer reachable here — derived classes must call
    // cleanup() in their own destructor (see MSpriteParticleRenderer).
    cleanup();
}

void MParticleRendererBase::cleanup()
{
    if (registered)
    {
        if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
            renderer->getRenderQueue().removeFromSubmitables(this);
        registered = false;
    }

    destroyGeometry();

    if (instanceVBO != 0)
    {
        glDeleteBuffers(1, &instanceVBO);
        instanceVBO      = 0;
        instanceCapacity = 0;
    }

    delete material;
    material       = nullptr;
    materialShader = nullptr;
}

void MParticleRendererBase::destroyGeometry()
{
    for (const SGeometry& g : geometry)
        if (g.vao != 0) glDeleteVertexArrays(1, &g.vao);
    geometry.clear();
}

int MParticleRendererBase::getShaderAlignmentMode() const
{
    const auto a = static_cast<EParticleAlignment>(alignment.get());
    if (useBillboards())
    {
        if (a == EParticleAlignment::Velocity) return 1;   // velocity billboard
        if (a == EParticleAlignment::ZAxis)    return 2;   // flat in simulation space
        return 0;                                          // face the camera
    }
    // Meshes: View and Z Axis both mean "use the particle rotation".
    return a == EParticleAlignment::Velocity ? 3 : 2;
}

SParticleFieldMeta MParticleRendererBase::getFieldMeta(const std::string& fieldName) const
{
    SParticleFieldMeta meta;
    if (fieldName == "alignment")
    {
        meta.options = { "View", "Velocity", "Z Axis" };
        meta.tooltip = "How each particle is turned.\n"
                       "View: sprites face the camera (meshes: rotation only).\n"
                       "Velocity: sprites point up along their movement while facing the camera;\n"
                       "meshes point +Z along their movement.\n"
                       "Z Axis: face the emitter's +Z (world +Z in Global space), turned by rotation.";
        return meta;
    }
    if (fieldName == "texture")
        meta.tooltip = "Multiplied with the particle colour. Empty: billboards draw a soft dot, meshes a flat colour.";
    else if (fieldName == "blendMode")
    {
        meta.options = { "Alpha", "Additive" };
        meta.tooltip = "Alpha: normal transparency, sorted back to front.\n"
                       "Additive: adds light (fire, sparks, magic) — order doesn't matter, never sorted.";
    }
    else if (fieldName == "lit")
        meta.tooltip = "Receive scene lighting. Off draws the colour at full brightness (usual for effects).";
    else if (fieldName == "sortByDistance")
        meta.tooltip = "Alpha blend only: draw far particles first so near ones blend over them.\n"
                       "Costs a sort of every particle, every frame.";
    return meta;
}

// -----------------------------------------------------------------------------
// GPU resources
// -----------------------------------------------------------------------------

void MParticleRendererBase::ensureInstanceBuffer()
{
    if (instanceVBO == 0)
        glGenBuffers(1, &instanceVBO);
}

unsigned int MParticleRendererBase::createInstancedVAO(unsigned int vertexBuffer, unsigned int elementBuffer)
{
    ensureInstanceBuffer();

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // ---- Per vertex — same layout as MStaticMesh::prepareMesh -------------------
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex), reinterpret_cast<void*>(offsetof(SVertex, Position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex), reinterpret_cast<void*>(offsetof(SVertex, Normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex), reinterpret_cast<void*>(offsetof(SVertex, TexCoords)));

    // ---- Per instance — divisor 1: advance once per particle, not per vertex ---
    // The VAO remembers the buffer *name*. Re-allocating its storage with
    // glBufferData later (orphaning) doesn't break this.
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    constexpr GLsizei stride = sizeof(SParticleInstance);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(SParticleInstance, position)));
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(SParticleInstance, rotation)));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(SParticleInstance, size)));
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(SParticleInstance, color)));
    glVertexAttribDivisor(6, 1);
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(SParticleInstance, velocity)));
    glVertexAttribDivisor(7, 1);

    if (elementBuffer != 0)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);   // captured by the VAO

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vao;
}

void MParticleRendererBase::uploadInstances()
{
    const size_t count = instances.size();
    if (count == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // Grow to at least maxParticles so the buffer is sized once, not every
    // time a few more particles are alive.
    if (count > instanceCapacity)
    {
        const size_t poolSize = owner ? static_cast<size_t>(std::max(0, owner->maxParticles.get())) : 0;
        instanceCapacity = std::max(count, poolSize);
    }

    // Orphan + refill: glBufferData with nullptr hands the driver a fresh
    // block, so we never wait for last frame's draw to finish reading the
    // old one. Costs a re-allocation inside the driver (usually pooled).
    const GLsizeiptr capacityBytes = static_cast<GLsizeiptr>(instanceCapacity * sizeof(SParticleInstance));
    glBufferData(GL_ARRAY_BUFFER, capacityBytes, nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(count * sizeof(SParticleInstance)), instances.data());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// -----------------------------------------------------------------------------
// Material
// -----------------------------------------------------------------------------

bool MParticleRendererBase::syncMaterial()
{
    // Looked up every frame (a map lookup) so a hot-reloaded shader is picked up.
    const auto shaderAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
        ->getAsset<MShaderAsset>(PARTICLE_SHADER);
    MShader* shader = shaderAsset ? shaderAsset->getShader() : nullptr;
    if (!shader) return false;

    const MMaterial::ShadingMode shading = lit.get() ? MMaterial::ShadingMode::Lit : MMaterial::ShadingMode::Unlit;
    if (!material)
    {
        material = mnew<MMaterial>(shader, shading);
        material->setDoubleSided(true);                               // quads seen from behind, thin meshes
        material->setRenderMode(MMaterial::RenderMode::Transparent);  // only the transparent stage draws instanced items
    }
    else if (shader != materialShader)
    {
        material->updateShader(shader, shading);
    }
    materialShader = shader;

    material->setBlendMode(blendMode.get() == 1 ? MMaterial::BlendMode::Additive : MMaterial::BlendMode::Alpha);

    // Texture reference in the form materials store: "guid:<id>" or a path.
    const auto& ref = texture.get();
    ref.resolve();
    const bool    hasTexture = !ref.getAssetId().empty() || !ref.getPath().empty();
    const SString refStr     = !ref.getAssetId().empty() ? SString(ASSET_REF_GUID_PREFIX) + ref.getAssetId()
                                                         : ref.getPath();

    SShaderPropertyValue v;
    v.setTextureReference(refStr);               material->setProperty("_albedo", v);
    v = {}; v.setBoolValue(hasTexture);          material->setProperty("_useAlbedoTex", v);
    v = {}; v.setColVal(glm::vec4(1.0f));        material->setProperty("_color", v);
    // _lit drives the shading mode the transparent stage reads (MMaterial::getShadingMode).
    v = {}; v.setBoolValue(lit.get());           material->setProperty("_lit", v);
    v = {}; v.setBoolValue(useBillboards());     material->setProperty("_isSprite", v);
    v = {}; v.setIntVal(getShaderAlignmentMode()); material->setProperty("_alignment", v);
    v = {}; v.setFloatVal(0.0f);                 material->setProperty("_metallic", v);
    v = {}; v.setFloatVal(0.0f);                 material->setProperty("_smoothness", v);
    return true;
}

// -----------------------------------------------------------------------------
// Per frame
// -----------------------------------------------------------------------------

bool MParticleRendererBase::canDraw()
{
    return owner && owner->isEnabledInHierarchy() && owner->getAliveCount() > 0;
}

void MParticleRendererBase::prepare(std::span<const SParticleData> particles)
{
    const size_t count = particles.size();
    instances.resize(count);
    drawOrder.resize(count);
    std::iota(drawOrder.begin(), drawOrder.end(), 0u);

    const SParticleSimContext ctx = owner->getSimContext();

    // ---- Sort back to front (alpha only) -----------------------------------------
    // Additive blending is order-independent (a + b == b + a), so it skips this.
    const bool alpha = blendMode.get() != 1;
    if (alpha && sortByDistance.get() && count > 1)
    {
        auto* views  = MEngineSubsystemRegistry::getSubsystem<MViewManagement>();
        auto* camera = views ? views->getFirstActiveCamera() : nullptr;   // same camera the pipeline culls with
        if (camera)
        {
            // Camera position in simulation space, so particles need no transform.
            SVector3 cam = camera->getWorldPosition();
            if (ctx.space == EParticleSimulationSpace::Local)
                cam = SVector3(glm::inverse(ctx.emitterToWorld) * SVector4(cam, 1.0f));

            sortKeys.resize(count);
            for (size_t i = 0; i < count; ++i)
            {
                const SVector3 d = particles[i].position - cam;
                sortKeys[i] = glm::dot(d, d);
            }
            // O(n log n) every frame — the main CPU cost of alpha particles.
            std::sort(drawOrder.begin(), drawOrder.end(),
                      [this](uint32_t a, uint32_t b) { return sortKeys[a] > sortKeys[b]; });   // farthest first
        }
    }

    // ---- Pack + bounds ---------------------------------------------------------------
    SVector3 mn(FLT_MAX), mx(-FLT_MAX);
    float    maxExtent = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        const SParticleData& p = particles[drawOrder[i]];
        SParticleInstance&   o = instances[i];
        o.position = p.position;
        o.rotation = SVector4(p.rotation.x, p.rotation.y, p.rotation.z, p.rotation.w);
        o.size     = p.size;
        o.color    = p.color;
        o.velocity = p.velocity;   // +12 bytes per particle — only read in Velocity alignment

        mn = glm::min(mn, p.position);
        mx = glm::max(mx, p.position);
        maxExtent = std::max(maxExtent, glm::max(p.size.x, glm::max(p.size.y, p.size.z)));
    }

    // Grow by the largest particle's reach. sqrt(3)/2 covers a unit cube
    // (or quad) in any rotation; for meshes larger than a unit cube this is
    // only approximate.
    const SVector3 pad(maxExtent * 0.8661f);
    mn -= pad;
    mx += pad;

    if (ctx.space == EParticleSimulationSpace::Local)
    {
        // Simulation-space box → world-space box around its 8 corners.
        SVector3 wmn(FLT_MAX), wmx(-FLT_MAX);
        for (int c = 0; c < 8; ++c)
        {
            const SVector3 corner((c & 1) ? mx.x : mn.x, (c & 2) ? mx.y : mn.y, (c & 4) ? mx.z : mn.z);
            const SVector3 w = SVector3(ctx.emitterToWorld * SVector4(corner, 1.0f));
            wmn = glm::min(wmn, w);
            wmx = glm::max(wmx, w);
        }
        mn = wmn;
        mx = wmx;
    }

    if (count == 0) { mn = mx = SVector3(0.0f); }
    worldBounds.min = mn;
    worldBounds.max = mx;

    uploadInstances();
}

void MParticleRendererBase::submitRenderItem(IRenderItemCollector* collector)
{
    if (!owner) return;
    if (!syncMaterial()) return;
    ensureInstanceBuffer();
    if (!syncGeometry() || geometry.empty()) return;

    const std::span<const SParticleData> particles = owner->getAliveParticles();
    if (particles.empty()) return;
    prepare(particles);

    const SParticleSimContext ctx = owner->getSimContext();
    const SMatrix4 model = ctx.space == EParticleSimulationSpace::Local ? ctx.emitterToWorld : SMatrix4(1.0f);

    for (const SGeometry& g : geometry)
    {
        SRenderItem item;
        item.vao           = g.vao;
        item.ebo           = g.ebo;
        item.indexCount    = g.indexCount;
        item.vertexCount   = g.vertexCount;
        item.instanceCount = static_cast<int>(particles.size());
        item.transform     = model;
        item.material      = material;
        item.bounds        = worldBounds;
        item.castsShadow   = false;   // transparent — the shadow pass never sees it
        item.sortOrder     = static_cast<int>(ERenderStageOrder::RS_Transparent);
        collector->submitRenderItem(item);
    }
}
