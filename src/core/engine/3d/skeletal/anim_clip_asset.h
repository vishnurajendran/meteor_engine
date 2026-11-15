//
// anim_clip_asset.h
//
// MAnimClipAsset - a model file imported with importAs = AnimationOnly:
// every clip (take) in it. Clips bind to any skeleton by exact bone name.
//
#pragma once
#ifndef ANIM_CLIP_ASSET_H
#define ANIM_CLIP_ASSET_H

#include <vector>

#include "anim_clip_source.h"
#include "core/engine/3d/model/model_import_settings.h"
#include "core/engine/assetmanagement/asset/asset.h"

class MAnimClip;

SCRIPT_BIND_CLASS()
class MAnimClipAsset : public MAsset, public IAnimationClipSource
{
    DEFINE_OBJECT_SUBCLASS(MAnimClipAsset)
public:
    MAnimClipAsset(const SString& path, const SModelImportSettings& settings);
    ~MAnimClipAsset() override;

    SCRIPT_BIND_FUNC()
    std::vector<SString> getClipNames() const;
    SCRIPT_BIND_FUNC()
    int getClipCount() const { return static_cast<int>(clips.size()); }

    // IAnimationClipSource
    const std::vector<MAnimClip*>& getClips() const override { return clips; }
    [[nodiscard]] uint64_t getClipGeneration() const override { return generation; }

    const SModelImportSettings& getImportSettings() const { return importSettings; }
    SModelImportSettings&       getImportSettings()       { return importSettings; }
    bool saveImportSettings() { return importSettings.saveToMeta(path); }

    bool requestReload() override { load(); return valid; }

private:
    void load();   // keeps the old clips on failure

    SModelImportSettings    importSettings;
    std::vector<MAnimClip*> clips;
    uint64_t                generation = 0;
};

#endif // ANIM_CLIP_ASSET_H
