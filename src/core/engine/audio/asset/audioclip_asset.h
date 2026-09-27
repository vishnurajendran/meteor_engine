//
// Created by ssj5v on 17-05-2026.
//

#ifndef AUDIOCLIP_ASSET_H
#define AUDIOCLIP_ASSET_H

#include "core/engine/assetmanagement/asset/asset.h"
#include "pugixml.hpp"

class IAudioClip;
class IAudioEngineSubsystem;

SCRIPT_BIND_CLASS()
class MAudioClipAsset : public MAsset {
    DEFINE_OBJECT_SUBCLASS(MAudioClipAsset)

public:
    explicit MAudioClipAsset(const SString& path);
    ~MAudioClipAsset() override;

    // Drops the runtime clip; the next getAudioClip() re-reads metadata
    // through the asset source. Sources pick up the new audio the next time
    // they bind the clip (MAudioSource::play / setClip).
    [[nodiscard]] bool requestReload() override;
    [[nodiscard]] bool hasDeferredLoad() const override { return false; }

    // Persist the preload flag back to the asset's meta (via the writable
    // asset source; fails cleanly in read-only / packaged builds).
    [[nodiscard]] bool save() override;

    // Called by the importer after construction to apply settings
    // that were stored in the asset's .meta XML file.
    void loadSettings(const pugi::xml_document& metaData);

    // Returns the runtime clip, creating it lazily through the audio engine
    // on first call. Returns nullptr if the engine is unavailable.
    IAudioClip* getAudioClip();

    // Release the clip through the engine. Safe to call multiple times.
    void releaseClip();

    // Convenience -- reads / writes the preload flag.
    bool isPreloaded() const;
    void setPreloaded(bool preload);

private:
    IAudioClip*            audioClip       = nullptr;
    IAudioEngineSubsystem* cachedEngine    = nullptr;
    bool                   preloadSetting  = false;
};

#endif //AUDIOCLIP_ASSET_H