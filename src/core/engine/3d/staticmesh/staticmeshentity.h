#pragma once
#ifndef STATICMESHENTITY_H
#define STATICMESHENTITY_H

#include "../../../graphics/core/render-pipeline/interfaces/drawable_interface.h"
#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/utils/aabb.h"
#include "staticmeshasset.h"

class MStaticMeshEntity : public MSpatialEntity, public IMeteorDrawable
{
    DEFINE_SPATIAL_CLASS(MStaticMeshEntity)

    DECLARE_FIELD(meshAssetPath,     std::string, "")
    DECLARE_FIELD(materialAssetPath, std::string, "")  // slot 0 — kept for backward compat
    DECLARE_FIELD(castsShadow,       bool,        true)

public:
    MStaticMeshEntity();
    ~MStaticMeshEntity() override;

    void submitRenderItem(IRenderItemCollector* collector) override;
    bool canDraw() override { return getEnabled(); }

    void onDrawGizmo(SVector2 renderResolution) override;
    void onExit()           override;
    void onUpdate(float dt) override;

    void setStaticMeshAsset(TAssetHandle<MStaticMeshAsset> asset);
    void setMaterialAsset(TAssetHandle<MMaterialAsset> asset, int slotId = 0);
    void swapMaterialSlots(int a, int b);
    void calculateBounds();

    [[nodiscard]] TAssetHandle<MStaticMeshAsset> getStaticMeshAsset()   const { return staticMeshAsset; }
    [[nodiscard]] TAssetHandle<MMaterialAsset>   getMaterialAsset(int slotId = 0)    const;
    [[nodiscard]] MMaterial*        getMaterialInstance(int slotId = 0) const;
    [[nodiscard]] int               getMaterialSlotCount()              const { return (int)materialSlots.size(); }
    [[nodiscard]] AABB              getBounds()                         const { return bounds; }
    [[nodiscard]] bool              getCastsShadow()                    const { return castsShadow.get(); }
    void setCastsShadow(bool v) { castsShadow = v; }

protected:
    void onSerialise(pugi::xml_node& node)         override;
    void onDeserialise(const pugi::xml_node& node) override;

private:
    struct SMaterialSlot
    {
        TAssetHandle<MMaterialAsset> assetHandle;

        [[nodiscard]] bool      isValid()     const { return assetHandle.isValid(); }
        [[nodiscard]] MMaterial* getMaterial() const
        {
            return isValid() ? assetHandle->getMaterial() : nullptr;
        }
    };

private:
    TAssetHandle<MStaticMeshAsset> staticMeshAsset;
    std::vector<SMaterialSlot>     materialSlots;
    AABB                           bounds              = { {0,0,0}, {0,0,0} };
    SMatrix4                       prevTransformMatrix{};
    MMaterial*                     errorMaterialInstance = nullptr;
};

#endif