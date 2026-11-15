//
// material_slots_drawer.h
//
// The "N Material Slots" section shared by the static and skeletal mesh
// inspectors: one asset reference per slot, drag a slot label onto another to
// swap them.
//
#pragma once
#ifndef MATERIAL_SLOTS_DRAWER_H
#define MATERIAL_SLOTS_DRAWER_H

#include <vector>

#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/graphics/core/material/MMaterialAsset.h"

class MAssetReferenceControl;
class MMeshEntityBase;

class MMaterialSlotsDrawer
{
public:
    void draw(MMeshEntityBase* entity);

private:
    struct SSlotControls
    {
        MAssetReferenceControl*      assetRef = nullptr;
        TAssetHandle<MMaterialAsset> lastKnownAsset;
    };

    void ensureSlotControls(int count);

    std::vector<SSlotControls> slotControls;
};

#endif // MATERIAL_SLOTS_DRAWER_H
