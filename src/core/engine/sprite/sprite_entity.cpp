//
// Sprite entity - a textured quad in the world.
//

#include "sprite_entity.h"

#include <cfloat>
#include <glm/gtc/matrix_transform.hpp>

#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/texture.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager_interface.h"
#include "core/graphics/core/render-pipeline/render_queue.h"
#include "core/graphics/core/render-pipeline/stages/render_stage.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/memory/mnew.h"

IMPLEMENT_SPATIAL_CLASS(MSpriteEntity)

// Opaque sprites: alpha cutout, lit by the deferred pass.
static constexpr const char* SPRITE_SHADER      = "meteor_assets/engine_assets/shaders/default/sprite.mesl";
// Transparent sprites: the standard forward-lit transparent shader. Its
// property names match (_albedo, _color, _metallic, _smoothness).
static constexpr const char* TRANSPARENT_SHADER = "meteor_assets/engine_assets/shaders/default/transparent.mesl";

// One unit quad shared by every sprite: -0.5..0.5 in XY, facing +Z.
// UV v = 0 is the top of the image, matching how meshes load (aiProcess_FlipUVs).
// Created on first use, when a GL context is guaranteed; lives for the program.
static const MStaticMesh* getUnitQuad()
{
    static MStaticMesh* quad = nullptr;
    if (!quad)
    {
        const SVector3 n(0.0f, 0.0f, 1.0f);
        std::vector<SVertex> vertices = {
            { SVector3(-0.5f, -0.5f, 0.0f), n, SVector2(0.0f, 1.0f) },   // bottom left
            { SVector3( 0.5f, -0.5f, 0.0f), n, SVector2(1.0f, 1.0f) },   // bottom right
            { SVector3( 0.5f,  0.5f, 0.0f), n, SVector2(1.0f, 0.0f) },   // top right
            { SVector3(-0.5f,  0.5f, 0.0f), n, SVector2(0.0f, 0.0f) },   // top left
        };
        std::vector<unsigned int> indices = { 0, 1, 2,  0, 2, 3 };   // CCW from +Z
        quad = mnew<MStaticMesh>(vertices, indices);
    }
    return quad;
}

MSpriteEntity::MSpriteEntity()
{
    name = "Sprite";
    if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
        renderer->getRenderQueue().addToSubmitables(this);
}

MSpriteEntity::~MSpriteEntity()
{
    if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
        renderer->getRenderQueue().removeFromSubmitables(this);
    delete material;
    material = nullptr;
}

void MSpriteEntity::onExit()
{
    MSpatialEntity::onExit();
    if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
        renderer->getRenderQueue().removeFromSubmitables(this);
}

// -- size & transform ----------------------------------------------------------

SVector2 MSpriteEntity::getSize() const
{
    const float ppu = glm::max(pixelsPerUnit.get(), 0.001f);
    if (auto* asset = texture.get().resolve())
        if (auto* tex = asset->getTexture())
        {
            const SVector2 px = tex->getSize();
            if (px.x > 0.0f && px.y > 0.0f)
                return px / ppu;
        }
    return SVector2(1.0f);
}

SMatrix4 MSpriteEntity::getQuadMatrix() const
{
    const SVector2 size = getSize();
    const SVector3 quadScale(size.x * (flipX.get() ? -1.0f : 1.0f),
                             size.y * (flipY.get() ? -1.0f : 1.0f),
                             1.0f);
    return glm::scale(getTransformMatrix(), quadScale);
}

// -- material ------------------------------------------------------------------

bool MSpriteEntity::syncMaterial()
{
    const bool blend = transparent.get();

    // Fetched every frame (a map lookup) so a hot-reloaded shader or a mode
    // change is picked up.
    const auto shaderAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
        ->getAsset<MShaderAsset>(blend ? TRANSPARENT_SHADER : SPRITE_SHADER);
    MShader* shader = shaderAsset ? shaderAsset->getShader() : nullptr;
    if (!shader) return false;

    if (!material)
    {
        material = mnew<MMaterial>(shader, MMaterial::ShadingMode::Lit);
        material->setDoubleSided(true);
        material->setBlendMode(MMaterial::BlendMode::Alpha);
    }
    else if (shader != materialShader)
    {
        material->updateShader(shader, MMaterial::ShadingMode::Lit);
    }
    materialShader = shader;

    material->setRenderMode(blend ? MMaterial::RenderMode::Transparent : MMaterial::RenderMode::Opaque);

    // Texture reference in the form materials store: "guid:<id>" or a path.
    const auto& ref = texture.get();
    ref.resolve();   // fills in the GUID for path-only refs
    const SString refStr = !ref.getAssetId().empty()
        ? SString(ASSET_REF_GUID_PREFIX) + ref.getAssetId()
        : ref.getPath();

    SShaderPropertyValue v;
    v.setTextureReference(refStr);                                    material->setProperty("_albedo", v);
    v = {}; v.setColVal(glm::vec4(color.get(), opacity.get()));       material->setProperty("_color", v);
    // _lit drives the shading mode for both shaders (MMaterial::getShadingMode).
    v = {}; v.setBoolValue(lit.get());                                 material->setProperty("_lit", v);
    v = {}; v.setBoolValue(true);                                      material->setProperty("_useAlbedoTex", v);
    v = {}; v.setFloatVal(0.0f);                                       material->setProperty("_smoothness", v);
    // Cutout only in opaque mode; blended sprites keep their soft edges.
    v = {}; v.setBoolValue(!blend);                                    material->setProperty("_useCutout", v);
    v = {}; v.setFloatVal(glm::clamp(cutoutThreshold.get(), 0.0f, 1.0f)); material->setProperty("_cutoutThreshold", v);
    return true;
}

// -- rendering -------------------------------------------------------------------

void MSpriteEntity::submitRenderItem(IRenderItemCollector* collector)
{
    if (!isEnabledInHierarchy()) return;
    if (!syncMaterial()) return;

    const MStaticMesh* quad = getUnitQuad();
    const SMatrix4 world = getQuadMatrix();

    // World bounds of the four corners.
    SVector3 mn(FLT_MAX), mx(-FLT_MAX);
    for (const auto& vert : quad->getVertices())
    {
        const SVector3 wp = SVector3(world * SVector4(vert.Position, 1.0f));
        mn = glm::min(mn, wp);
        mx = glm::max(mx, wp);
    }
    bounds.min = mn;
    bounds.max = mx;

    SRenderItem item;
    item.vao         = quad->getVAO();
    item.ebo         = quad->getEBO();
    item.indexCount  = quad->getIndexCount();
    item.vertexCount = quad->getVertexCount();
    item.transform   = world;
    item.material    = material;
    item.bounds      = bounds;
    item.castsShadow = castsShadow.get() && !transparent.get();
    item.sortOrder   = static_cast<int>(material->isTransparent() ? ERenderStageOrder::RS_Transparent
                                                                  : ERenderStageOrder::RS_Opaque);
    collector->submitRenderItem(item);
}
