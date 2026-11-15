//
// anim_clip.h
//
// MAnimClip - one animation clip, plain data shared by MSkeletalMeshAsset
// (embedded clips) and MAnimClipAsset (animation-only files).
//
// Every track is resampled on import to evenly spaced keys at sampleRate, so
// sampling is an index (i = t * rate) plus a blend between two keys - no key
// search. A track whose channel never changes keeps a single key.
//
// A clip has one track per animated bone, keyed by bone name. It binds to any
// skeleton by exact name (see bone_binding.h), so animation-only files work
// without a shared skeleton asset.
//
#pragma once
#ifndef ANIM_CLIP_H
#define ANIM_CLIP_H

#include <vector>

#include "core/object/object.h"
#include "core/utils/transform.h"
#include "skeleton.h"

class MAnimClip : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MAnimClip)
public:
    struct STrack
    {
        SString                  boneName;
        // Each holds frameCount keys, or exactly 1 for a constant channel.
        std::vector<SVector3>    positions;
        std::vector<SQuaternion> rotations;
        std::vector<SVector3>    scales;
    };

    MAnimClip() = default;
    ~MAnimClip() override = default;

    // getName() is the clip name used by playClip.
    [[nodiscard]] float getDuration()   const { return duration; }
    [[nodiscard]] float getSampleRate() const { return sampleRate; }
    [[nodiscard]] int   getFrameCount() const { return frameCount; }
    [[nodiscard]] int   getTrackCount() const { return static_cast<int>(tracks.size()); }
    [[nodiscard]] const STrack& getTrack(int i) const { return tracks[i]; }
    [[nodiscard]] const std::vector<STrack>& getTracks() const { return tracks; }

    // Path of the asset the clip came from - for status messages.
    [[nodiscard]] const SString& getSourcePath() const { return sourcePath; }

    [[nodiscard]] uint64_t getId() const { return id; }

    // Local transform of one track at `time` seconds. `time` must already be
    // wrapped or clamped to [0, duration].
    [[nodiscard]] STransform sampleTrack(int trackIndex, float time) const;

    // -- Building (importer only) -------------------------------------------------
    void setup(const SString& clipName, float durationSeconds, float rate, int frames, const SString& source)
    {
        name       = clipName;
        duration   = durationSeconds;
        sampleRate = rate;
        frameCount = frames;
        sourcePath = source;
    }
    std::vector<STrack>& editTracks() { return tracks; }

private:
    float               duration   = 0.0f;
    float               sampleRate = 30.0f;
    int                 frameCount = 1;
    SString             sourcePath;
    std::vector<STrack> tracks;
    uint64_t            id = nextAnimationDataId();
};

#endif // ANIM_CLIP_H
