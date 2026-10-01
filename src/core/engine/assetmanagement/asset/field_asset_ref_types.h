//
// field_asset_ref_types.h
//
// Field<T> partial specialization for TAssetRef<AssetType>.
//
// Include this in any header that uses DECLARE_FIELD with a TAssetRef,
// AFTER both data/field.h and asset_ref_handle.h are visible.
// Follows the same pattern as field_engine_types.h.
//
// Serializes the reference as a single string:
//   "guid:<id>"  — when the asset's GUID is known (always the case once resolved)
//   "<path>"     — fallback when the GUID isn't known (e.g. the asset is missing)
//
// XML format:
//   <fieldName>guid:3d4c5ab2-7f32-...</fieldName>
//   <fieldName>assets/meshes/cube.obj</fieldName>      (hand-written / unresolved)
//
// Legacy format — still read, never written:
//   <fieldName>
//       <id>guid-string</id>
//       <path>assets/meshes/cube.obj</path>
//   </fieldName>
//

#ifndef FIELD_ASSET_REF_TYPES_H
#define FIELD_ASSET_REF_TYPES_H

#include "data/field.h"
#include "core/engine/assetmanagement/asset/asset_ref_handle.h"
#include "pugixml.hpp"

template <typename T>
struct Field<TAssetRef<T>> : public FieldBase
{
    TAssetRef<T> rawValue;

    Field(std::vector<FieldBase*>& registry, const std::string& n, const TAssetRef<T>& def)
        : FieldBase(registry, n), rawValue(def)
    {}

    const TAssetRef<T>& get() const { return rawValue; }
    TAssetRef<T>&       get()       { return rawValue; }

    // kinda hacky but f*ck it
    TAssetHandle<T> getHandle() const
    {
        auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
        if (assetManager == nullptr)
            return TAssetHandle<T>();

        return assetManager->getAssetById<T>(rawValue.getAssetId());
    }

    void set(const TAssetRef<T>& v)
    {
        rawValue = v;
        if (onChangeCallback)
            onChangeCallback(rawValue);
    }

    void setOnChangeCallback(std::function<void(const TAssetRef<T>&)> v) { onChangeCallback = v; }

    Field& operator=(const TAssetRef<T>& v) { rawValue = v; return *this; }

    // Writes "guid:<id>" when possible, otherwise the bare path.
    void write(pugi::xml_node& parent) const override
    {
        // Upgrade path-only references to a GUID before saving. resolve()
        // backfills m_assetId on a successful path lookup — one map lookup
        // per reference per save.
        rawValue.resolve();

        SString ref;
        if (!rawValue.getAssetId().empty())
            ref = SString(ASSET_REF_GUID_PREFIX) + rawValue.getAssetId();
        else
            ref = rawValue.getPath();

        parent.append_child(name.c_str()).text().set(ref.c_str());
    }

    // Reads either the new single-string form or the legacy <id>/<path> form.
    // Missing nodes are silently ignored.
    void load(const pugi::xml_node& parent) override
    {
        auto node = parent.child(name.c_str());
        if (!node) return;

        const auto idNode   = node.child("id");
        const auto pathNode = node.child("path");

        if (idNode || pathNode)
        {
            // Legacy format.
            if (idNode)   rawValue.setAssetId(SString(idNode.text().as_string("")));
            if (pathNode) rawValue.setPath(SString(pathNode.text().as_string("")));
        }
        else
        {
            const SString ref(node.text().as_string(""));
            if (IAssetManagerSubsystem::isGuidReference(ref))
            {
                rawValue.setAssetId(SString(ref.str().substr(ASSET_REF_GUID_PREFIX_LEN)));
                rawValue.setPath(SString{});
            }
            else
            {
                rawValue.setAssetId(SString{});
                rawValue.setPath(ref);
            }
        }

        // Fill in whichever half is missing (path for the inspector, GUID for
        // fast lookups). Scenes load after the asset database, so this succeeds
        // for any asset that exists.
        rawValue.resolve();
    }

private:
    std::function<void(const TAssetRef<T>&)> onChangeCallback;
};

#endif // FIELD_ASSET_REF_TYPES_H