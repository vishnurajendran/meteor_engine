//
// animation_player.h
//
// SAnimationPlayer - playback state for one character: the active clips
// (up to 3, crossfading), their times and weights, speed.
//
// A plain struct, not part of the entity. playClip drives it today; a state
// machine later becomes another driver of the same struct, with no change to
// the pose jobs or skinning.
//
// Crossfades:
//   - play() adds an entry whose weight rises from 0 to 1 over blendTime; the
//     others fall proportionally (each keeps its share of what is left).
//   - Entries at weight 0 are dropped. A fourth entry mid-blend drops the
//     oldest.
//   - Playing the clip that is already the newest entry is a no-op.
//
// The player never dereferences clip pointers itself: it keeps each entry's
// name and duration, so it is safe to query while clip pointers are stale
// (the subsystem refreshes them before every evaluation).
//
#pragma once
#ifndef ANIMATION_PLAYER_H
#define ANIMATION_PLAYER_H

#include <array>
#include <vector>

#include "core/utils/sstring.h"

class MAnimClip;
struct SClipBinding;

struct SAnimationPlayerEntry
{
    SString             clipName;
    const MAnimClip*    clip     = nullptr;   // refreshed by the owner before evaluation
    const SClipBinding* binding  = nullptr;
    float               duration = 0.0f;
    float               time     = 0.0f;
    float               weight   = 0.0f;
    float               baseWeight = 0.0f;    // share of the fade-out, set when a fade starts
    bool                loop     = true;
    bool                finished = false;     // a non-looping clip reached its end (reported once)
};

struct SAnimationPlayer
{
    static constexpr int MAX_ENTRIES = 3;

    std::array<SAnimationPlayerEntry, MAX_ENTRIES> entries{};
    int   entryCount = 0;
    float speed      = 1.0f;

    // Fade state. With a target, the newest entry fades in to 1 while the
    // rest fade out; without one (stop()), everything fades out.
    bool  hasFadeTarget = false;
    float fadeProgress  = 1.0f;   // 0..1
    float fadeRate      = 0.0f;   // progress per second

    // Returns false when it was a no-op (the clip is already the newest entry).
    bool play(const SString& clipName, const MAnimClip* clip, const SClipBinding* binding,
              float duration, float blendTime, bool loop);
    void stop(float blendTime);

    // Advances clip times and fades. Names of non-looping clips that reached
    // their end this call are appended to `finishedClips`.
    void advance(float deltaTime, std::vector<SString>& finishedClips);

    // Drops every entry `shouldRemove` accepts (e.g. a clip that disappeared
    // on reload), renormalizing the rest.
    template <typename Pred>
    void removeEntriesIf(Pred shouldRemove)
    {
        const bool hadTarget = hasFadeTarget && entryCount > 0;
        bool targetRemoved = false;
        int write = 0;
        for (int i = 0; i < entryCount; ++i)
        {
            if (shouldRemove(entries[i]))
            {
                targetRemoved |= hadTarget && i == entryCount - 1;
                continue;
            }
            if (write != i) entries[write] = entries[i];
            ++write;
        }
        if (write != entryCount)
        {
            entryCount = write;
            afterRemoval(targetRemoved);
        }
    }

    [[nodiscard]] bool isPlaying() const { return entryCount > 0; }
    // The entry with the highest weight, or -1.
    [[nodiscard]] int  dominantEntry() const;

    void clear() { entryCount = 0; hasFadeTarget = false; fadeProgress = 1.0f; fadeRate = 0.0f; }

private:
    void startFade(float blendTime, bool withTarget);
    void applyFadeWeights();
    void dropEntry(int index);
    void normalizeWeights();
    // Keeps a running fade consistent after entries were removed.
    void afterRemoval(bool targetRemoved);
};

#endif // ANIMATION_PLAYER_H
