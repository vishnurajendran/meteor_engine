//
// Created by ssj5v on 27-03-2025.
//

#include "skybox.h"

#include "core/engine/3d/shader/shaderasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/graphics/core/graphicsrenderer.h"
#include "core/graphics/core/render_queue.h"
MSkyboxEntity::MSkyboxEntity()
{
    auto shaderAsset = MAssetManager::getInstance()->getAsset<MShaderAsset>("meteor_assets/engine_assets/shaders/skybox.mesl");
    if (!shaderAsset)
    {
        MERROR("SkyboxEntity::Failed to load shader asset");
        return;
    }
    skyboxDrawCall = new MSkyboxDrawCall(nullptr, shaderAsset->getShader());
    MRenderQueue::addToSubmitables(this);
    name = "Skybox";
}

void MSkyboxEntity::setCubemapAsset(MCubemapAsset* cubemap)
{
    if (!skyboxDrawCall) return;
    this->skyboxDrawCall->setCubemapAsset(cubemap);
}

void MSkyboxEntity::prepareForDraw()
{
    //nothing here
}

void MSkyboxEntity::raiseDrawCall()
{
    MGraphicsRenderer::submit(skyboxDrawCall);
}
