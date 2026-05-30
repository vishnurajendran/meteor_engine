//
// Created by ssj5v on 18-05-2026.
//

#ifndef AUDIO_ENTITY_H
#define AUDIO_ENTITY_H
#include "core/engine/audio/asset/audioclip_asset.h"
#include "core/engine/audio/interfaces/audiosource_interface.h"
#include "core/engine/audio/interfaces/engine_interface.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/assetmanagement/asset/field_asset_ref_types.h"

class MAudioSource : public MSpatialEntity {
    DEFINE_SPATIAL_CLASS(MAudioSource)

    // Fields

    // Audio Controls
    DECLARE_FIELD(loop, bool, false)
    DECLARE_FIELD(volume, float, 1.0f)
    DECLARE_FIELD(pitch, float, 1.0f)
    DECLARE_FIELD(autoStart, bool, true);

    // Spatialization
    DECLARE_FIELD(useSpatial, bool, false)
    DECLARE_FIELD(rollOff, float, 1.0f)
    DECLARE_FIELD(minDist, float, 10.0f)
    DECLARE_FIELD(maxDist, float, 100.0f)
    DECLARE_FIELD(dopplerStrength, float, 0.0f)

    // Asset reference -- auto-serialized to XML via Field<TAssetRef<T>>
    DECLARE_FIELD(clipRef, TAssetRef<MAudioClipAsset>, {})

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
    /// Set clip to play -- accepts a handle; TAssetRef converts implicitly
    void setClip(TAssetHandle<MAudioClipAsset> clip);
    /// Get current clip reference (carries both GUID and path)
    TAssetRef<MAudioClipAsset> getClip() const { return clipRef.get(); }
    /// Play this source
    void play();
    /// Stop this source
    void stop();
    /// Play this clip as oneshot
    void playOneShot(TAssetHandle<MAudioClipAsset> clip);

    // -- Audio control accessors (used by the inspector) ----------------------
    bool isPlaying() const { return sourcePlaying; }

    bool getAutoStart() const { return autoStart.get(); }
    void setAutoStart(const bool& as) { autoStart.set(as); }

    bool  getLoop()   const { return loop.get(); }
    void  setLoop(const bool&  v)   { loop.set(v); }

    float getVolume() const { return volume.get(); }
    void  setVolume(const float&  v){ volume.set(v); }

    float getPitch()  const { return pitch.get(); }
    void  setPitch(const float&  v) { pitch.set(v); }

    // -- Spatialization accessors ---------------------------------------------

    bool  getUseSpatial()   const { return useSpatial.get(); }
    void  setUseSpatial(bool v)   { useSpatial.set(v); }

    float getRollOff()      const { return rollOff.get(); }
    void  setRollOff(float v)     { rollOff.set(v); }

    float getMinDist()      const { return minDist.get(); }
    void  setMinDist(float v)     { minDist.set(v); }

    float getMaxDist()      const { return maxDist.get(); }
    void  setMaxDist(float v)     { maxDist.set(v); }

    float getDopplerStrength()  const { return dopplerStrength.get(); }
    void  setDopplerStrength(float v) { dopplerStrength.set(v); }

private:
    void syncAudioEngineState();

private:
    IAudioEngineSubsystem* engineSubsystem = nullptr;
    IAudioSource* source = nullptr;
    bool initialized = false;

    // velocity
    SVector3 prevPos = {0,0,0};

    // initially out of sync, the next play syncs the state.
    bool outOfSync = true;
    bool sourcePlaying = false;
};



#endif //AUDIO_ENTITY_H