//
// Created by ssj5v on 17-05-2026.
//

#ifndef AUDIOCLIP_ASSET_H
#define AUDIOCLIP_ASSET_H

#include "core/engine/assetmanagement/asset/asset.h"

class MAudioClipAsset : public MAsset {
    DEFINE_OBJECT_SUBCLASS(MAudioClipAsset)

public:
    explicit MAudioClipAsset(const SString& path);
    ~MAudioClipAsset() override = default;

    [[nodiscard]] bool requestReload() override { return true; }
    [[nodiscard]] bool hasDeferredLoad() const override { return false; }
};

#endif //AUDIOCLIP_ASSET_H