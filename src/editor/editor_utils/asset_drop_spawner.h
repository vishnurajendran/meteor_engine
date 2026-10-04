//
// asset_drop_spawner.h
//
// Turns an asset dragged from the asset browser into an entity in the scene.
// Shared by the scene view and the hierarchy so both create exactly the same
// thing for the same asset:
//
//   static mesh  (.glb …)  →  MStaticMeshEntity with the mesh assigned
//   audio clip   (.mp3 …)  →  MAudioSource with the clip assigned
//   composition  (.comp)   →  a linked composition instance
//
// The new entity is created at the scene root and selected. Callers then move
// it where they need it (world position under the mouse, child of a row …).
//

#ifndef ASSET_DROP_SPAWNER_H
#define ASSET_DROP_SPAWNER_H

#include "core/utils/sstring.h"

class MSpatialEntity;

class MAssetDropSpawner
{
public:
    // True if spawn() can create something for this asset id — lets drop
    // targets ignore assets they can't place (textures, shaders, …).
    [[nodiscard]] static bool canSpawn(const SString& assetId);

    // Creates the entity for the asset at the scene root and selects it.
    // Returns nullptr for unsupported assets or when no scene is open.
    static MSpatialEntity* spawn(const SString& assetId);
};

#endif // ASSET_DROP_SPAWNER_H
