//
// animation_subsystem.h
//
// MAnimationSubsystem - evaluates every skeletal mesh's pose and skins it,
// once per frame.
//
// Like physics, canTick() stays false: the applications call update()
// directly, between the scene's update and lateUpdate. A playClip from a
// script's onTick then takes effect the same frame, and lateUpdate code
// (cameras, attached weapons) sees this frame's bones.
//
//   onTick -> animation -> bone modifiers -> onPostAnimate (pose editable)
//          -> sockets -> skinning -> onLateTick (pose read-only, final)
//
// Each frame:
//   1. Main thread: resolve meshes, clip tables and clip pointers, advance
//      clip times and crossfades, decide what each instance does this frame
//      (culling), report finished clips.
//   2. Jobs (one task per instance): sample, blend, walk bones parent-first
//      to local / model / skinning matrices and the joint box. Each task
//      writes only its own instance.
//   3. Main thread: bone modifiers (parent-first), then onPostAnimate.
//   4. Main thread: re-walk instances whose bones changed in 3.
//   5. Main thread: bone sockets, so they see the edited pose.
//   6. Main thread: one upload of every instance's skinning matrices, one
//      dispatch per visible sub-mesh, one barrier.
//
// Skinning runs once per frame, not per view: the editor's scene view, game
// view and thumbnails all draw the same output buffers.
//
// Runtime data lives here, not on the entity: an MSkeletalMeshEntity holds a
// handle into the instance pool, registering on construction and
// unregistering on exit / destruction (like its render queue registration).
//
#pragma once
#ifndef ANIMATION_SUBSYSTEM_H
#define ANIMATION_SUBSYSTEM_H

#include <memory>
#include <vector>

#include "core/engine/subsystem/subsystem_interface.h"
#include "core/object/object.h"
#include "core/utils/frustum.h"
#include "pose.h"
#include "skinning_backend.h"

class MSkeletalMeshEntity;

class MAnimationSubsystem : public MObject, public IEngineSubSystem
{
    DEFINE_OBJECT_SUBCLASS(MAnimationSubsystem)
public:
    MAnimationSubsystem();
    ~MAnimationSubsystem() override;

    void init() override {}
    void cleanup() override;

    // The frame. Main thread, GL context current.
    void update(float deltaTime);

    // -- Instances ----------------------------------------------------------------
    SAnimInstanceHandle registerInstance(MSkeletalMeshEntity* owner);
    void                unregisterInstance(SAnimInstanceHandle& handle);   // resets the handle
    SAnimInstance*       getInstance(const SAnimInstanceHandle& handle);
    const SAnimInstance* getInstance(const SAnimInstanceHandle& handle) const;

    [[nodiscard]] int getInstanceCount() const { return aliveCount; }

    // True between steps 3 and 6 - pose writes are only meaningful inside
    // onPostAnimate, which runs then.
    [[nodiscard]] bool isRunningPoseModifiers() const { return runningModifiers; }

    // -- Skinning backend ---------------------------------------------------------
    void setSkinningBackend(ESkinningBackend type);
    [[nodiscard]] ESkinningBackend getSkinningBackend() const { return backendType; }

    // Re-walks model and skinning matrices from local transforms (parent-first)
    // and the joint box. Exposed for the pose API.
    static void computeModelSpace(SAnimInstance& instance);

private:
    void ensureBackend();
    void buildFrustums();
    [[nodiscard]] bool isVisible(const AABB& bounds) const;
    static void evaluatePose(SAnimInstance& instance);
    void runModifiers(SAnimInstance& instance, float deltaTime);
    void compactPool();

    std::vector<std::unique_ptr<SAnimInstance>> pool;
    std::vector<uint32_t>                       freeSlots;
    int                                         aliveCount = 0;
    bool                                        updating   = false;
    bool                                        hasDeadSlots = false;
    bool                                        runningModifiers = false;

    std::vector<SAnimInstance*> evaluated;     // this frame, in pool order
    std::vector<SFrustum>       frustums;
    std::vector<SMatrix4>       boneMatrices;  // every skinned instance's matrices, back to back
    std::vector<SSkinningJob>   skinJobs;

    struct SFinishedClip
    {
        SAnimInstanceHandle handle;
        SString             clipName;
    };
    std::vector<SFinishedClip> finishedClips;
    std::vector<SString>       finishedScratch;

    std::unique_ptr<ISkinningBackend> backend;
    ESkinningBackend                  backendType = ESkinningBackend::GPU;
    bool                              backendReady = false;
};

#endif // ANIMATION_SUBSYSTEM_H
