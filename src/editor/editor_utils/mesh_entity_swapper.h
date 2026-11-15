//
// mesh_entity_swapper.h
//
// After a model's importAs changes, the asset is a different class and the
// entities that used it resolve to nothing (a TAssetRef<MStaticMeshAsset> to
// a now-skeletal file is null, never cast). This finds those entities in the
// open scene and swaps each for the matching entity type, keeping its name,
// parent, transform, material slots, script and children.
//
#pragma once
#ifndef MESH_ENTITY_SWAPPER_H
#define MESH_ENTITY_SWAPPER_H

#include "core/utils/sstring.h"

namespace MMeshEntitySwapper
{
    // Entities in the active scene whose mesh reference points at the asset
    // but are the wrong entity type for its current class.
    int countMismatched(const SString& assetId);

    // Swaps them. Returns how many were swapped.
    int swapMismatched(const SString& assetId);
}

#endif // MESH_ENTITY_SWAPPER_H
