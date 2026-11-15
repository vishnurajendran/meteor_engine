//
// skeleton.h
//
// SSkeleton - the bone hierarchy of a skeletal mesh. Plain data, owned by
// MSkeletalMeshAsset in v1.
//
// Bones are stored parent-first in depth-first pre-order, so:
//   - a bone's parent always has a lower index (walk 0..n to build model space)
//   - a bone's subtree is the contiguous range [i, subtreeEnd)
//
#pragma once
#ifndef SKELETON_H
#define SKELETON_H

#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "core/utils/glmhelper.h"
#include "core/utils/sstring.h"
#include "core/utils/sstring_hash.h"
#include "core/utils/transform.h"

// Unique, never-reused ids for skeletons and clips. Bone-binding caches key on
// these instead of pointers, so a reloaded asset that happens to land at the
// same address never picks up a stale table.
inline uint64_t nextAnimationDataId()
{
    static std::atomic<uint64_t> counter{1};
    return counter.fetch_add(1, std::memory_order_relaxed);
}

struct SBone
{
    SString    name;                // exactly as authored - no prefix stripping or case folding
    int        parent      = -1;    // -1 for roots
    int        subtreeEnd  = 0;     // one past the last descendant
    SMatrix4   inverseBind = SMatrix4(1.0f);   // model space -> bone space, at bind
    STransform bindPose;            // local (parent-relative) bind transform
    bool       isDeformBone = false;   // referenced by a mesh's skin weights
};

struct SSkeleton
{
    std::vector<SBone> bones;

    // Applied in front of every root bone's transform. Holds the import pivot,
    // so it moves the character without shifting the bind-space vertices.
    SMatrix4 rootOffset = SMatrix4(1.0f);

    // The first deform bone that has no deform-bone ancestor. Clips that have
    // no track for it can't be played on this skeleton (status Unavailable).
    int rootBone = -1;

    uint64_t id = nextAnimationDataId();

    [[nodiscard]] int getBoneCount() const { return static_cast<int>(bones.size()); }

    // -1 when there is no bone of that exact name.
    [[nodiscard]] int findBone(const SString& boneName) const
    {
        const auto it = nameToIndex.find(boneName);
        return it == nameToIndex.end() ? -1 : it->second;
    }

    // Call after filling `bones` (and whenever names change).
    void rebuildLookup()
    {
        nameToIndex.clear();
        nameToIndex.reserve(bones.size());
        for (int i = 0; i < static_cast<int>(bones.size()); ++i)
            nameToIndex.try_emplace(bones[i].name, i);   // first bone of a duplicated name wins
    }

    // Model-space bind transforms (rootOffset applied), parent-first.
    void computeBindModelTransforms(std::vector<SMatrix4>& out) const
    {
        out.resize(bones.size());
        for (size_t i = 0; i < bones.size(); ++i)
        {
            const SMatrix4 local = bones[i].bindPose.toMatrix();
            out[i] = bones[i].parent < 0 ? rootOffset * local : out[bones[i].parent] * local;
        }
    }

private:
    std::unordered_map<SString, int> nameToIndex;
};

#endif // SKELETON_H
