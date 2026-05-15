//
// skybox.h
//
#pragma once
#ifndef SKYBOXENTITIY_H
#define SKYBOXENTITIY_H

#include "../../graphics/core/render-pipeline/stages/skybox/skyboxdrawcall.h"
#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/graphics/core/render-pipeline/interfaces/drawable_interface.h"
#include "cubemapasset.h"

class MCubemapAsset;

class MSkyboxEntity : public MSpatialEntity, public IMeteorDrawable
{
    DEFINE_SPATIAL_CLASS(MSkyboxEntity)

    // Asset reference serialized as a path string.
    // onDeserialise loads the actual asset from this path.
    DECLARE_FIELD(cubemapAssetPath, std::string, "")

public:
    MSkyboxEntity();
    ~MSkyboxEntity() override;

    TAssetHandle<MCubemapAsset> getCubemapAsset() const { return cubemapAsset; }
    void setCubemapAsset(TAssetHandle<MCubemapAsset> cubemap);

    // ── IMeteorDrawable ───────────────────────────────────────────────────────
    void submitRenderItem(IRenderItemCollector* collector) override {}
    bool canDraw() override { return getEnabled(); }

    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

protected:
    void onDeserialise(const pugi::xml_node& node) override;

private:
    MSkyboxDrawCall* skyboxDrawCall = nullptr;
    TAssetHandle<MCubemapAsset>  cubemapAsset;
};

#endif // SKYBOXENTITIY_H