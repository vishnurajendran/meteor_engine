//
// bone_binding.h
//
// How a clip's tracks map onto a skeleton's bones.
//
// The first time a clip is used on a skeleton, a table mapping each track to
// a bone index is built and cached per (clip, skeleton) pair. All name
// lookups go through resolveBoneIndex(), so a later version can add name
// mapping (MRetargetAsset) without touching anything else.
//
// v1 matches names exactly: tracks with no matching bone are ignored, bones
// with no track stay in bind pose.
//
#pragma once
#ifndef BONE_BINDING_H
#define BONE_BINDING_H

#include <vector>

#include "core/utils/sstring.h"

class MAnimClip;
struct SSkeleton;

enum class EClipStatus : int
{
    Available   = 0,   // every deform bone has a track
    Warning     = 1,   // some bones matched - normal for upper-body clips, extra socket/IK/face bones
    Unavailable = 2,   // no bones matched, or the skeleton's root bone has no track
    Overridden  = 3,   // a later source has a clip with the same name
};

struct SClipBinding
{
    std::vector<int> trackToBone;      // per track; -1 = no such bone (ignored)
    int  matchedBones   = 0;           // deform bones that have a track
    int  totalBones     = 0;           // deform bones in the skeleton
    bool rootHasTrack   = false;
    EClipStatus status  = EClipStatus::Unavailable;   // Available / Warning / Unavailable only

    [[nodiscard]] bool isPlayable() const { return status == EClipStatus::Available || status == EClipStatus::Warning; }
};

namespace MBoneBinding
{
    // The one place a track name becomes a bone index. -1 when no bone matches.
    int resolveBoneIndex(const SSkeleton& skeleton, const SString& trackName);

    // Cached per (clip, skeleton). The reference stays valid for the life of
    // the program (entries are never erased, and are keyed by ids that are
    // never reused). Main thread only - jobs read the result, never build it.
    const SClipBinding& getBinding(const MAnimClip& clip, const SSkeleton& skeleton);

    // Human-readable reason for an Unavailable binding, e.g.
    // "no bones match Hero skeleton". Empty when playable.
    SString describeUnavailable(const SClipBinding& binding, const SSkeleton& skeleton, const SString& skeletonName);
}

#endif // BONE_BINDING_H
