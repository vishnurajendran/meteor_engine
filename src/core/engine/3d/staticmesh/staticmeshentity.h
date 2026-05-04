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
    DEFINE_CLASS(MStaticMeshEntity)

    DECLARE_FIELD(meshAssetPath,     std::string, "")
    DECLARE_FIELD(materialAssetPath, std::string, "")  // slot 0 path for scene serialisation
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
    void calculateBounds();

    [[nodiscard]] MStaticMeshAsset* getStaticMeshAsset()                const { return staticMeshAsset.get(); }
    [[nodiscard]] MMaterialAsset*   getMaterialAsset(int slotId = 0)    const;
    // Returns the material from the asset directly (no instancing).
    [[nodiscard]] MMaterial*        getMaterialInstance(int slotId = 0) const;
    [[nodiscard]] int               getMaterialSlotCount()              const { return (int)materialSlots.size(); }
    [[nodiscard]] AABB              getBounds()                         const { return bounds; }
    [[nodiscard]] bool              getCastsShadow()                    const { return castsShadow.get(); }
    void setCastsShadow(bool v) { castsShadow = v; }

protected:
    void onDeserialise(const pugi::xml_node& node) override;

private:
    // Simple slot — just holds a handle. No owned MMaterial*.
    // All users of the same MMaterialAsset share its original material directly.
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