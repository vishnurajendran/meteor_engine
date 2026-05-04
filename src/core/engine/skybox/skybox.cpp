//
// skybox.cpp
//
#include "skybox.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/render-pipeline/stages/skybox/skybox_queue.h"
#include "core/graphics/core/shader/shaderasset.h"

IMPLEMENT_CLASS(MSkyboxEntity)

MSkyboxEntity::MSkyboxEntity()
{
    name = "Skybox";

    auto* shaderAsset = MAssetManager::getInstance()
        ->getAsset<MShaderAsset>("meteor_assets/engine_assets/shaders/internal/skybox.mesl");
    if (!shaderAsset)
    {
        MERROR("MSkyboxEntity — failed to load skybox shader asset");
        return;
    }

    skyboxDrawCall = new MSkyboxDrawCall(nullptr, shaderAsset->getShader());
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

void MSkyboxEntity::setCubemapAsset(MCubemapAsset* cubemap)
{
    cubemapAsset = cubemap;
    // Keep path field in sync so it serializes correctly
    if (cubemap)
        cubemapAssetPath = cubemap->getPath().str();
    if (skyboxDrawCall)
        skyboxDrawCall->setCubemapAsset(cubemap);
}

void MSkyboxEntity::onDeserialise(const pugi::xml_node& node)
{
    // Let base class load fields (including cubemapAssetPath)
    MSpatialEntity::onDeserialise(node);

    // Resolve the asset path → actual asset
    const std::string& path = cubemapAssetPath.get();
    if (!path.empty())
    {
        auto* asset = MAssetManager::getInstance()->getAsset<MCubemapAsset>(path.c_str());
        if (asset)
            setCubemapAsset(asset);
        else
            MWARN(STR("MSkyboxEntity: could not load cubemap asset at path: ") + path);
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
    auto* texture = MAssetManager::getInstance()
        ->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/skybox.png");
    if (texture)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f), texture->getTexture());
}