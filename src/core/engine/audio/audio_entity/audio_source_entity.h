//
// Created by ssj5v on 18-05-2026.
//

#ifndef AUDIO_ENTITY_H
#define AUDIO_ENTITY_H
#include "core/engine/audio/interfaces/audiosource_interface.h"
#include "core/engine/audio/interfaces/engine_interface.h"
#include "core/engine/entities/spatial/spatial.h"

class MAudioSource : public MSpatialEntity {
    DEFINE_SPATIAL_CLASS(MAudioSource)

    // Fields

    // Audio Controls
    DECLARE_FIELD(loop, bool, false)
    DECLARE_FIELD(volume, float, 1.0f)
    DECLARE_FIELD(pitch, float, 1.0f)

    // Spatialization
    DECLARE_FIELD(useSpatial, bool, false)
    DECLARE_FIELD(rollOff, float, 1.0f)
    DECLARE_FIELD(minDist, float, 10.0f)
    DECLARE_FIELD(maxDist, float, 100.0f)
    DECLARE_FIELD(dopplerStrength, float, 0.0f)

public:
    MAudioSource() = default;
    ~MAudioSource() override = default;

    void onCreate() override;
    void onStart() override;
    void onUpdate(float deltaTime) override;
    void onExit() override;
    void onDrawGizmo(SVector2 renderResolution) override;

// Public API
public:
    /// Set clip to play
    void setClip(TAssetHandle<MAudioClipAsset> clip);
    /// Get current clip
    TAssetHandle<MAudioClipAsset> getClip() const { return clipRef; }
    /// Play this source
    void play();
    /// Stop this source
    void stop();
    /// Play this clip as oneshot
    void playOneShot(TAssetHandle<MAudioClipAsset> clip);
private:
    void syncAudioEngineState();

private:
    IAudioEngineSubsystem* engineSubsystem = nullptr;
    IAudioSource* source = nullptr;
    bool initialized = false;

    // velocity
    SVector3 prevPos = {0,0,0};
    TAssetHandle<MAudioClipAsset> clipRef;

    // initially out of sync, the next play syncs the state.
    bool outOfSync = true;
};



#endif //AUDIO_ENTITY_H
