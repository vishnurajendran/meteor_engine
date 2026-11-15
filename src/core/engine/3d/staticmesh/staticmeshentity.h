#pragma once
#ifndef STATICMESHENTITY_H
#define STATICMESHENTITY_H

#include "core/engine/3d/common/mesh_entity_base.h"
#include "core/engine/assetmanagement/asset/asset_ref_handle.h"
#include "staticmeshasset.h"

SCRIPT_BIND_CLASS()
class MStaticMeshEntity : public MMeshEntityBase
{
    DEFINE_SPATIAL_CLASS(MStaticMeshEntity)

    // Serialized via Field<TAssetRef<T>> -- writes both GUID and path to XML.
    // Replaces the old meshAssetPath (std::string) + separate TAssetHandle pair.
    DECLARE_FIELD(meshAsset, TAssetRef<MStaticMeshAsset>, {})
public:
    MStaticMeshEntity();
    ~MStaticMeshEntity() override = default;

    void submitRenderItem(IRenderItemCollector* collector) override;
    void onUpdate(float dt) override;

    SCRIPT_BIND_FUNC()
    void setStaticMeshAsset(TAssetRef<MStaticMeshAsset> asset);
    void calculateBounds();

    // Returns a TAssetHandle for API compatibility with existing callers.
    SCRIPT_BIND_FUNC()
    TAssetHandle<MStaticMeshAsset> getStaticMeshAsset() const { return meshAsset.get().getHandle(); }

protected:
    void onSerialise(pugi::xml_node& node)         override;
    void onDeserialise(const pugi::xml_node& node) override;

private:
    // The reference is set but resolves to nothing. Logs once if the asset
    // exists as another type - e.g. a model whose importAs was switched to
    // SkeletalMesh - so it isn't mistaken for a missing file.
    void warnIfWrongAssetType();

    SMatrix4 prevTransformMatrix{};
    bool     warnedWrongType = false;
};

#endif
