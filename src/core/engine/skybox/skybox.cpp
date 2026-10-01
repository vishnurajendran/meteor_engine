//
// skybox.cpp
//
#include "skybox.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/stages/skybox/skybox_queue.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "default_engine_icon_paths.h"

IMPLEMENT_SPATIAL_CLASS(MSkyboxEntity)

MSkyboxEntity::MSkyboxEntity()
{
    name = "Skybox";

    const auto shaderAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
        ->getAsset<MShaderAsset>("meteor_assets/engine_assets/shaders/internal/skybox.mesl");
    if (!shaderAsset)
    {
        MERROR("MSkyboxEntity - failed to load skybox shader asset");
        return;
    }

    skyboxDrawCall = new MSkyboxDrawCall(TAssetHandle<MCubemapAsset>(), shaderAsset->getShader());
    MSkyboxQueue::add(skyboxDrawCall);
}

MSkyboxEntity::~MSkyboxEntity()
{
    if (skyboxDrawCall)
    {
        MSkyboxQueue::remove(skyboxDrawCall);
        delete skyboxDrawCall;
        skyboxDrawCall = nullptr;
    }
}

void MSkyboxEntity::setCubemapAsset(TAssetHandle<MCubemapAsset> cubemap)
{
    cubemapAsset = cubemap;
    // Keep the reference field in sync so it serializes correctly — "guid:<id>"
    if (cubemap)
        cubemapAssetPath = cubemap.toRefString().str();
    if (skyboxDrawCall)
        skyboxDrawCall->setCubemapAsset(cubemap);
}

void MSkyboxEntity::onDeserialise(const pugi::xml_node& node)
{
    // Let base class load fields (including cubemapAssetPath)
    MSpatialEntity::onDeserialise(node);

    // Resolve the reference ("guid:<id>" or path) → actual asset.
    // setCubemapAsset() rewrites the field as "guid:<id>", so old path-based
    // scenes are upgraded on the next save.
    const std::string& ref = cubemapAssetPath.get();
    if (!ref.empty())
    {
        const auto asset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAssetFromReference<MCubemapAsset>(ref.c_str());
        if (asset)
            setCubemapAsset(asset);
        else
            MWARN(STR("MSkyboxEntity: could not load cubemap asset: ") + ref);
    }
}

void MSkyboxEntity::onExit()
{
    MSpatialEntity::onExit();
    if (skyboxDrawCall)
        MSkyboxQueue::remove(skyboxDrawCall);
}

void MSkyboxEntity::onDrawGizmo(SVector2 renderResolution)
{
    const auto texture = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
        ->getAsset<MTextureAsset>(SEngineAssetIconPaths::HIGHRES_TEX_GIZMOS_SKYBOX);
    if (texture)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f), texture->getTexture());
}