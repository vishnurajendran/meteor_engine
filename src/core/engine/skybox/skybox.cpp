//
// Created by ssj5v on 27-03-2025.
//

#include "skybox.h"
#include "core/graphics/core/render-pipeline/stages/skybox/skybox_queue.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"

MSkyboxEntity::MSkyboxEntity()
{
    name = "Skybox";

    auto* shaderAsset = MAssetManager::getInstance()
                            ->getAsset<MShaderAsset>(
                                "meteor_assets/engine_assets/shaders/skybox.mesl");
    if (!shaderAsset)
    {
        MERROR("MSkyboxEntity — failed to load skybox shader asset");
        return;
    }

    skyboxDrawCall = new MSkyboxDrawCall(nullptr, shaderAsset->getShader());

    // Register with the skybox-specific queue so MSkyboxStage picks it up.
    // The skybox does not go through MRenderQueue because it doesn't fit the
    // standard SRenderItem contract (custom shader, cubemap, depth state).
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

void MSkyboxEntity::onExit()
{
    MSpatialEntity::onExit();   // was missing — keeps base-class state consistent
    if (skyboxDrawCall)
        MSkyboxQueue::remove(skyboxDrawCall);
}

void MSkyboxEntity::setCubemapAsset(MCubemapAsset* cubemap)
{
    cubemapAsset = cubemap;
    if (skyboxDrawCall)
        skyboxDrawCall->setCubemapAsset(cubemap);
}

void MSkyboxEntity::onDrawGizmo(SVector2 renderResolution)
{
    auto* texture = MAssetManager::getInstance()
                        ->getAsset<MTextureAsset>(
                            "meteor_assets/engine_assets/icons/skybox.png");
    if (texture)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f),
                                 texture->getTexture());
}