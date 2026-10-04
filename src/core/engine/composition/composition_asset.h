//
// composition_asset.h
//
// A .comp file — a serialised entity subtree that can be placed in a scene
// many times (a "prefab"). The content is exactly what serialiseEntity()
// writes, wrapped in a <composition> root:
//
//   <composition formatVersion="1">
//       <entity type="MStaticMeshEntity" name="House" ...>
//           ...fields...
//           <children> ... </children>
//       </entity>
//   </composition>
//
// The asset itself knows nothing about MSpatialEntity — it only stores XML and
// a content hash. Building / saving / resetting instances lives in
// MCompositionUtility (composition_utility.h). Keeping this header free of
// spatial.h lets MSpatialEntity declare a TAssetRef<MCompositionAsset> field
// without an include cycle.
//

#ifndef COMPOSITION_ASSET_H
#define COMPOSITION_ASSET_H

#include "core/engine/assetmanagement/asset/asset.h"
#include "pugixml.hpp"

class MCompositionAsset : public MAsset
{
    DEFINE_OBJECT_SUBCLASS(MCompositionAsset)

public:
    static constexpr const char* FILE_EXTENSION = "comp";
    static constexpr const char* ROOT_TAG       = "composition";

    explicit MCompositionAsset(const SString& path);
    ~MCompositionAsset() override = default;

    // Re-reads the file. If the content changed, clean instances in the open
    // scene are rebuilt (see MCompositionUtility::refreshInstances).
    bool requestReload() override;

    // The <entity> node of the composition root — empty node if invalid.
    [[nodiscard]] pugi::xml_node getRootEntityNode() const;

    // Hash of the normalised content (see computeContentHash). Instances
    // remember the hash they were built from to know when they are stale.
    [[nodiscard]] const SString& getContentHash() const { return contentHash; }

    // Replaces this composition's content with `entityNode` (a serialised
    // subtree), writes the file and reloads. Used by an instance's Save.
    bool writeContent(const pugi::xml_node& entityNode);

    // ---- Static helpers -------------------------------------------------------

    // Builds the on-disk document for `entityNode`:
    //   — strips composition links from every node (nested instances are
    //     flattened — a .comp never references another .comp)
    //   — moves the root to the origin (instances own their root position)
    static void buildFileDocument(const pugi::xml_node& entityNode, pugi::xml_document& out);

    // Writes a brand-new .comp file. Does not register it with the asset
    // manager — the editor does that (MEditorAssetManager::onFileAdded).
    static bool writeFile(const SString& path, const pugi::xml_node& entityNode);

    // Stable hash (FNV-1a 64) of an entity subtree with everything that is
    // per-instance removed: the root's name, enabled flag and transform, plus
    // composition link data on every node. Two subtrees with the same hash
    // have the same composition content.
    static SString computeContentHash(const pugi::xml_node& entityNode);

    // Names of the per-instance link data written on an instance root.
    static constexpr const char* LINK_FIELD_NAME       = "compAssetReference";
    static constexpr const char* SOURCE_HASH_ATTRIBUTE = "compSourceHash";
    static constexpr const char* STATE_HASH_ATTRIBUTE  = "compStateHash";

private:
    bool loadFromSource();
    static bool saveDocument(const SString& path, const pugi::xml_document& doc);

    pugi::xml_document document;   // owned by value, like MSceneAsset
    SString            contentHash;
};

#endif // COMPOSITION_ASSET_H
