#pragma once
#ifndef STATICMESHENTITY_H
#define STATICMESHENTITY_H

#include "core/graphics/core/render-pipeline/interfaces/drawable_interface.h"
#include "core/engine/assetmanagement/asset/asset_ref_handle.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/utils/aabb.h"
#include "staticmeshasset.h"
#include "core/engine/assetmanagement/asset/asset_ref_handle.h"

class MStaticMeshEntity : public MSpatialEntity, public IMeteorDrawable
{
    DEFINE_SPATIAL_CLASS(MStaticMeshEntity)

    // Serialized via Field<TAssetRef<T>> -- writes both GUID and path to XML.
    // Replaces the old meshAssetPath (std::string) + separate TAssetHandle pair.
    DECLARE_FIELD(meshAsset, TAssetRef<MStaticMeshAsset>, {})
    DECLARE_FIELD(castsShadow, bool, true)
public:
    MStaticMeshEntity();
    ~MStaticMeshEntity() override;

    void submitRenderItem(IRenderItemCollector* collector) override;
    bool canDraw() override { return getEnabled(); }

    void onDrawGizmo(SVector2 renderResolution) override;
    void onExit()           override;
    void onUpdate(float dt) override;

    void setStaticMeshAsset(TAssetRef<MStaticMeshAsset> asset);
    void setMaterialAsset(TAssetRef<MMaterialAsset> asset, int slotId = 0);
    void swapMaterialSlots(int a, int b);
    void calculateBounds();

    // Returns a TAssetHandle for API compatibility with existing callers.
    [[nodiscard]] TAssetHandle<MStaticMeshAsset> getStaticMeshAsset()        const { return meshAsset.get().getHandle(); }
    [[nodiscard]] TAssetHandle<MMaterialAsset>   getMaterialAsset(int slotId = 0) const;
    [[nodiscard]] MMaterial*        getMaterialInstance(int slotId = 0) const;
    [[nodiscard]] int               getMaterialSlotCount()              const { return (int)materialSlots.size(); }
    [[nodiscard]] AABB              getBounds()                         const { return bounds; }
    [[nodiscard]] bool              getCastsShadow()                    const { return castsShadow.get(); }
    void setCastsShadow(bool v) { castsShadow = v; }


// Editor only API
#if METEOR_EDITOR
public:
    void setDrawGizmo(bool enabled) { debug_drawBounds = enabled; };
    bool getDrawGizmo() const { return debug_drawBounds; };
#endif

protected:
    void onSerialise(pugi::xml_node& node)         override;
    void onDeserialise(const pugi::xml_node& node) override;

private:
    // Each slot holds a serializable ref to a material asset.
    // Replaces the old TAssetHandle-only slot.
    struct SMaterialSlot
    {
        TAssetRef<MMaterialAsset> assetRef;

        [[nodiscard]] bool isValid() const { return assetRef.isValid(); }
        [[nodiscard]] MMaterial* getMaterial() const
        {
            auto* asset = assetRef.resolve();
            return asset ? asset->getMaterial() : nullptr;
        }
    };

private:
    std::vector<SMaterialSlot>     materialSlots;
    AABB                           bounds              = { {0,0,0}, {0,0,0} };
    SMatrix4                       prevTransformMatrix{};
    MMaterial*                     errorMaterialInstance = nullptr;

#if METEOR_EDITOR
    bool                           debug_drawBounds = false;
#endif
};

#endif