//
// mesh_entity_base.h
//
// Shared base for entities that draw a mesh asset with one material per
// sub-mesh: MStaticMeshEntity and MSkeletalMeshEntity.
//
// Owns the material slots (their serialization, including the legacy
// formats, and resizing them to the mesh's sub-mesh count), the cast-shadow
// flag, the world bounds and the error-material fallback. Subclasses own the
// mesh reference and build the render items.
//
#pragma once
#ifndef MESH_ENTITY_BASE_H
#define MESH_ENTITY_BASE_H

#include "core/engine/assetmanagement/asset/asset_ref_handle.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/render-pipeline/interfaces/drawable_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/utils/aabb.h"

SCRIPT_BIND_CLASS()
class MMeshEntityBase : public MSpatialEntity, public IMeteorDrawable
{
    DEFINE_ABSTRACT_SPATIAL_CLASS(MMeshEntityBase)

    DECLARE_FIELD(castsShadow, bool, true)

public:
    MMeshEntityBase();
    ~MMeshEntityBase() override;

    bool canDraw() override { return getEnabled(); }
    void onExit() override;

    // -- Material slots ---------------------------------------------------------
    SCRIPT_BIND_FUNC()
    void setMaterialAsset(TAssetRef<MMaterialAsset> asset, int slotId = 0);
    void swapMaterialSlots(int a, int b);

    SCRIPT_BIND_FUNC()
    TAssetHandle<MMaterialAsset> getMaterialAsset(int slotId = 0) const;
    MMaterial* getMaterialInstance(int slotId = 0) const;

    SCRIPT_BIND_FUNC()
    int getMaterialSlotCount() const { return (int)materialSlots.size(); }

    // -- Shadows / bounds -------------------------------------------------------
    SCRIPT_BIND_FUNC()
    bool getCastsShadow() const { return castsShadow.get(); }
    SCRIPT_BIND_FUNC()
    void setCastsShadow(bool v) { castsShadow = v; }

    SCRIPT_BIND_FUNC()
    AABB getBounds() const { return bounds; }

#if METEOR_EDITOR
    void setDrawGizmo(bool enabled) { debug_drawBounds = enabled; }
    bool getDrawGizmo() const { return debug_drawBounds; }
#endif

    void onDrawGizmo(SVector2 renderResolution) override;

protected:
    // Resizes the slot list to the mesh's sub-mesh count, keeping existing
    // assignments. A count of 0 (no mesh) leaves a single empty slot.
    void resizeMaterialSlots(int subMeshCount);

    // Writes <materialSlots> / reads it back, with every legacy format.
    void serialiseMaterialSlots(pugi::xml_node& node) const;
    void deserialiseMaterialSlots(const pugi::xml_node& node);

    // The slot's material, or the engine error material when the slot is
    // empty or its asset is missing. nullptr only if both are unavailable.
    MMaterial* resolveSlotMaterialOrError(int slotId);

    // Fills the fields every mesh render item shares (material, shadow,
    // bounds, sort order). Geometry and transform are left to the caller.
    void fillCommonRenderItem(SRenderItem& item, MMaterial* material) const;

    struct SMaterialSlot
    {
        TAssetRef<MMaterialAsset> assetRef;

        bool isValid() const { return assetRef.isValid(); }
        MMaterial* getMaterial() const
        {
            auto* asset = assetRef.resolve();
            return asset ? asset->getMaterial() : nullptr;
        }
    };

    std::vector<SMaterialSlot> materialSlots;
    AABB                       bounds = { {0,0,0}, {0,0,0} };

private:
    MMaterial* errorMaterialInstance = nullptr;

#if METEOR_EDITOR
    bool debug_drawBounds = false;
#endif
};

#endif // MESH_ENTITY_BASE_H
