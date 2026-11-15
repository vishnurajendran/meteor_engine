//
// anim_clip_source.h
//
// Anything that supplies animation clips: MSkeletalMeshAsset (its embedded
// clips) and MAnimClipAsset (an animation-only file). Entity code only sees
// this interface, so a clip in a character's own file can still be played on
// other characters.
//
#pragma once
#ifndef ANIM_CLIP_SOURCE_H
#define ANIM_CLIP_SOURCE_H

#include <cstdint>
#include <vector>

class MAnimClip;

class IAnimationClipSource
{
public:
    virtual ~IAnimationClipSource() = default;
    virtual const std::vector<MAnimClip*>& getClips() const = 0;

    // Changes every time the source (re)loads. Entities rebuild their clip
    // table when it changes.
    [[nodiscard]] virtual uint64_t getClipGeneration() const = 0;
};

#endif // ANIM_CLIP_SOURCE_H
