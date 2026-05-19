//
// field_asset_ref_types.h
//
// Field<T> partial specialization for TAssetRef<AssetType>.
//
// Include this in any header that uses DECLARE_FIELD with a TAssetRef,
// AFTER both data/field.h and asset_ref_handle.h are visible.
// Follows the same pattern as field_engine_types.h.
//
// Serializes both the asset GUID and file path into XML. On load, reads
// whichever is present. This supports three scenarios:
//   - New files: both <id> and <path> are present.
//   - Legacy files: only <path> is present (pre-GUID scene files).
//   - Copied directories: <path> is valid, <id> may not resolve yet.
//
// XML format:
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
    void                set(const TAssetRef<T>& v) { rawValue = v; }

    Field& operator=(const TAssetRef<T>& v) { rawValue = v; return *this; }

    // Write both GUID and path as child nodes.
    void write(pugi::xml_node& parent) const override
    {
        auto node = parent.append_child(name.c_str());
        node.append_child("id").text().set(rawValue.getAssetId().c_str());
        node.append_child("path").text().set(rawValue.getPath().c_str());
    }

    // Read whichever children are present. Missing nodes are silently
    // ignored, so legacy files with only <path> still load correctly.
    void load(const pugi::xml_node& parent) override
    {
        auto node = parent.child(name.c_str());
        if (!node) return;

        if (auto idNode = node.child("id"))
            rawValue.setAssetId(SString(idNode.text().as_string("")));

        if (auto pathNode = node.child("path"))
            rawValue.setPath(SString(pathNode.text().as_string("")));
    }
};

#endif // FIELD_ASSET_REF_TYPES_H