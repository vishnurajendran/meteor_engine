//
// Created by ssj5v on 27-03-2025.
//

#pragma once
#ifndef SKYBOXENTITIY_H
#define SKYBOXENTITIY_H

#include "../../graphics/core/render-pipeline/stages/skybox/skyboxdrawcall.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/graphics/core/render-pipeline/interfaces/drawable_interface.h"
#include "cubemapasset.h"

class MCubemapAsset;

class MSkyboxEntity : public MSpatialEntity, public IMeteorDrawable
{
public:
    MSkyboxEntity();
    ~MSkyboxEntity() override;

    MCubemapAsset* getCubemapAsset() const { return cubemapAsset; }
    SString typeName() const override { return STR("skybox"); }
    void setCubemapAsset(MCubemapAsset* cubemap);

    // ---- IMeteorDrawable ----------------------------------------------------
    // Skybox does not produce an SRenderItem — it registers its draw call
    // directly with MSkyboxQueue so MSkyboxStage can drive it with the correct
    // depth state and cubemap binding.
    void submitRenderItem(IRenderItemCollector* collector) override {}
    bool canDraw() override { return getEnabled(); }

    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

private:
    MSkyboxDrawCall* skyboxDrawCall = nullptr;
    MCubemapAsset*   cubemapAsset   = nullptr;
};

#endif // SKYBOXENTITIY_H