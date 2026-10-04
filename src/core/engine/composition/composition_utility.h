//
// composition_utility.h
//
// Everything that connects a .comp asset (MCompositionAsset) with entities in
// the scene. An "instance" is any entity whose compAssetReference field is set
// — it is the instance root, and its whole subtree comes from the asset.
//
// ── Model ────────────────────────────────────────────────────────────────────
//   — The .comp is the source of truth. An instance mirrors it, except for the
//     root's name, enabled flag and transform, which belong to the instance.
//   — No per-property overrides. Editing inside an instance makes it
//     "Modified" until you Save (push to asset) or Reset (pull from asset).
//   — Clearing compAssetReference (Unlink) breaks the link; the entities stay.
//
// ── Change tracking ──────────────────────────────────────────────────────────
//   Each instance root stores two hashes (see MCompositionAsset::computeContentHash):
//     compSourceHash — the asset's content hash the instance was built from.
//                      != asset hash  →  the asset changed since  →  "Out of date"
//     compStateHash  — the instance's own hash right after it was built/saved.
//                      != current hash →  edited since            →  "Modified"
//   Clean instances follow the asset automatically (on asset change and on
//   scene load). Modified instances are never overwritten — no lost work.
//

#ifndef COMPOSITION_UTILITY_H
#define COMPOSITION_UTILITY_H

#include <functional>
#include <vector>

#include "core/utils/sstring.h"
#include "pugixml.hpp"

class MSpatialEntity;
class MCompositionAsset;

enum class ECompositionInstanceState
{
    NotLinked,     // no compAssetReference
    MissingAsset,  // reference doesn't resolve (deleted / .meta lost)
    Synced,        // identical to the asset
    Modified,      // edited in the scene, not saved to the asset
    OutOfDate      // asset changed, instance not rebuilt yet
};

class MCompositionUtility
{
public:
    // ---- Queries ----------------------------------------------------------------

    // `entity` itself if it is an instance root, otherwise its nearest ancestor
    // that is. nullptr when `entity` isn't part of an instance.
    static MSpatialEntity* findInstanceRoot(MSpatialEntity* entity);

    // Resolved asset of an instance root (nullptr if not linked / missing).
    static MCompositionAsset* getAsset(const MSpatialEntity* instanceRoot);

    // Serialises the subtree to compare hashes — O(subtree size). Fine for the
    // selected entity in the inspector; don't call it for every entity per frame.
    static ECompositionInstanceState getState(MSpatialEntity* instanceRoot);

    static SString computeInstanceHash(MSpatialEntity* instanceRoot);

    // ---- Creating / linking -----------------------------------------------------

    // Writes `entity`'s subtree to a new .comp at `path` (does not register it).
    static bool writeNewAsset(MSpatialEntity* entity, const SString& path);

    // Makes `entity` an instance of `asset` (sets the reference and both hashes).
    static void link(MSpatialEntity* entity, MCompositionAsset* asset);

    // Breaks the link — the entities stay as plain entities.
    static void unlink(MSpatialEntity* instanceRoot);

    // Builds a new instance at the scene root. Returns the instance root.
    static MSpatialEntity* instantiate(MCompositionAsset* asset);

    // ---- Instance actions -------------------------------------------------------

    // Writes the instance's subtree to its .comp, then rebuilds every other
    // clean instance of that asset in the open scene.
    static bool saveInstance(MSpatialEntity* instanceRoot);

    // Rebuilds the instance from its .comp, keeping the root's name, enabled
    // flag, transform and hierarchy position. The old subtree is destroyed, so
    // the returned root is a NEW entity. Returns nullptr on failure.
    static MSpatialEntity* resetInstance(MSpatialEntity* instanceRoot);

    // Rebuilds every clean, out-of-date instance of `asset` in the open scene.
    // Called after Save and when the asset is hot-reloaded.
    static void refreshInstances(MCompositionAsset* asset);

    // ---- Scene loading ----------------------------------------------------------

    // Called by MSpatialEntity::deserialiseEntity for each scene node. If the
    // node is a clean instance whose asset has changed, builds it from the
    // asset instead of the scene snapshot and returns it. Otherwise returns
    // nullptr and the caller loads the snapshot as usual.
    static MSpatialEntity* tryBuildFromAsset(const pugi::xml_node& sceneNode);

    // ---- Editor hook ------------------------------------------------------------

    // Reset replaces entities; anything holding a pointer to the old root
    // (e.g. the editor selection) must be redirected. Set by the editor.
    static std::function<void(MSpatialEntity* oldRoot, MSpatialEntity* newRoot)> onInstanceReplaced;

private:
    static void destroySubtree(MSpatialEntity* entity);
    static void collectInstances(MSpatialEntity* entity, const SString& assetId,
                                 std::vector<MSpatialEntity*>& out);
};

#endif // COMPOSITION_UTILITY_H
