//
// pose.h
//
// SAnimInstance - the per-character runtime data in MAnimationSubsystem's
// instance pool, and SPoseEditContext - the view of it that bone modifiers
// and onPostAnimate get.
//
#pragma once
#ifndef ANIMATION_POSE_H
#define ANIMATION_POSE_H

#include <cstdint>
#include <vector>

#include "animation_player.h"
#include "core/utils/glmhelper.h"
#include "core/utils/transform.h"

class MAnimClip;
class MSkeletalMeshEntity;
struct SClipBinding;
struct SSkeleton;

struct SAnimInstanceHandle
{
    uint32_t index      = UINT32_MAX;
    uint32_t generation = 0;
    [[nodiscard]] bool isNull() const { return index == UINT32_MAX; }
};

struct SAnimInstance
{
    MSkeletalMeshEntity* owner      = nullptr;
    uint32_t             generation = 0;
    bool                 alive      = false;

    SAnimationPlayer player;

    // -- Filled on the main thread before the jobs (step 1) --------------------
    const SSkeleton* skeleton = nullptr;
    struct SEvalEntry
    {
        const MAnimClip*    clip    = nullptr;
        const SClipBinding* binding = nullptr;
        float               time    = 0.0f;
        float               weight  = 0.0f;
    };
    SEvalEntry evalEntries[SAnimationPlayer::MAX_ENTRIES];
    int        evalCount   = 0;

    bool evaluate = false;   // bones evaluated this frame
    bool skin     = false;   // skinned this frame (evaluate implies nothing about skin)
    bool visible  = true;    // inside a camera frustum (last frame's bounds)

    // -- Pose (written by this instance's job only) ----------------------------
    std::vector<STransform> local;       // parent-relative
    std::vector<SMatrix4>   model;       // relative to the mesh entity (rootOffset applied)
    std::vector<SMatrix4>   skinning;    // model * inverseBind - what the skinning shader reads
    uint64_t                skeletonId = 0;   // the skeleton `local` was sized for
    SVector3                jointMin = SVector3(0.0f);   // model space, deform joints
    SVector3                jointMax = SVector3(0.0f);
    bool                    hasPose  = false;   // evaluated at least once since the skeleton changed

    // Blend accumulators, kept to avoid per-frame allocation.
    std::vector<SVector3> accPosition;
    std::vector<SVector4> accRotation;
    std::vector<SVector3> accScale;
    std::vector<float>    accWeight;

    // -- Pose editing (main thread) ---------------------------------------------
    bool poseEditable = false;   // inside onPostAnimate
    bool poseDirty    = false;   // local changed after evaluation; model needs a re-walk

    // Offset of this instance's skinning matrices in the shared bone buffer.
    uint32_t boneOffset = 0;

    void resizeFor(int boneCount)
    {
        local.resize(boneCount);
        model.resize(boneCount);
        skinning.resize(boneCount);
        accPosition.resize(boneCount);
        accRotation.resize(boneCount);
        accScale.resize(boneCount);
        accWeight.resize(boneCount);
    }
};

// What a bone modifier (and the pose API) may touch. Local edits re-walk the
// edited bone's subtree straight away, so a modifier further down the
// hierarchy sees its ancestors' new model transforms.
class SPoseEditContext
{
public:
    SPoseEditContext(SAnimInstance& instance, const SSkeleton& skeleton, const SMatrix4& entityWorld)
        : instance(instance), skeleton(skeleton), entityWorld(entityWorld) {}

    [[nodiscard]] const SSkeleton& getSkeleton() const { return skeleton; }
    [[nodiscard]] int getBoneCount() const { return static_cast<int>(instance.local.size()); }

    [[nodiscard]] const STransform& getLocal(int bone) const { return instance.local[bone]; }
    [[nodiscard]] const SMatrix4&   getModel(int bone) const { return instance.model[bone]; }
    // The parent's model transform, or the skeleton's root offset for roots.
    [[nodiscard]] SMatrix4 getParentModel(int bone) const;

    // Sets a local transform and re-walks the bone's subtree.
    void setLocal(int bone, const STransform& transform);

    // The mesh entity's world matrix - to bring world targets into model space.
    [[nodiscard]] const SMatrix4& getEntityWorld() const { return entityWorld; }

private:
    SAnimInstance&   instance;
    const SSkeleton& skeleton;
    const SMatrix4&  entityWorld;
};

#endif // ANIMATION_POSE_H
