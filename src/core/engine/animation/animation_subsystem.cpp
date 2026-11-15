#include "animation_subsystem.h"

#include <algorithm>
#include <limits>
#include <span>

#include "bone_binding.h"
#include "bone_modifier.h"
#include "core/application/application.h"
#include "core/engine/3d/skeletal/anim_clip.h"
#include "core/engine/3d/skeletal/skeletal_mesh_entity.h"
#include "core/engine/3d/skeletal/skeleton.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/profiling/simple_profiler/simple_profiler.h"
#include "core/utils/logger.h"
#include "core/utils/parallel_for.h"
#include "cpu_skinning_backend.h"
#include "gpu_skinning_backend.h"
#include "pose_evaluation.h"

namespace
{
    // Frustum culling happens before any view is rendered, so the real
    // viewport sizes aren't known. A wide aspect keeps the test conservative.
    const SVector2 CULLING_RESOLUTION(2400.0f, 1000.0f);

    // Pose jobs: instances per task, and the instance count below which
    // evaluation stays on the calling thread.
    constexpr size_t POSE_JOB_CHUNK_SIZE = 1;
    constexpr size_t POSE_JOB_THRESHOLD  = 2;
}

// ---------------------------------------------------------------------------
// SPoseEditContext
// ---------------------------------------------------------------------------

SMatrix4 SPoseEditContext::getParentModel(int bone) const
{
    const int parent = skeleton.bones[bone].parent;
    return parent < 0 ? skeleton.rootOffset : instance.model[parent];
}

void SPoseEditContext::setLocal(int bone, const STransform& transform)
{
    if (bone < 0 || bone >= getBoneCount())
        return;
    instance.local[bone] = transform;

    // Re-walk the subtree now, so modifiers on descendants see this edit.
    const int end = std::min(skeleton.bones[bone].subtreeEnd, getBoneCount());
    for (int i = bone; i < end; ++i)
    {
        const int parent = skeleton.bones[i].parent;
        const SMatrix4 parentModel = parent < 0 ? skeleton.rootOffset : instance.model[parent];
        instance.model[i] = parentModel * instance.local[i].toMatrix();
    }
    instance.poseDirty = true;   // skinning matrices and the joint box follow in step 4
}

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

MAnimationSubsystem::MAnimationSubsystem() = default;

MAnimationSubsystem::~MAnimationSubsystem()
{
    cleanup();
}

void MAnimationSubsystem::cleanup()
{
    if (backend)
    {
        backend->cleanup();
        backend.reset();
    }
    backendReady = false;

    // Entities still alive keep a handle; detach them so they don't reach a
    // pool that is gone.
    for (auto& inst : pool)
        if (inst && inst->alive && inst->owner)
            inst->owner->handle = SAnimInstanceHandle{};
    pool.clear();
    freeSlots.clear();
    aliveCount = 0;
}

// ---------------------------------------------------------------------------
// Instances
// ---------------------------------------------------------------------------

SAnimInstanceHandle MAnimationSubsystem::registerInstance(MSkeletalMeshEntity* owner)
{
    uint32_t index;
    if (!freeSlots.empty() && !updating)
    {
        index = freeSlots.back();
        freeSlots.pop_back();
    }
    else
    {
        index = static_cast<uint32_t>(pool.size());
        pool.push_back(std::make_unique<SAnimInstance>());
    }

    SAnimInstance& inst = *pool[index];
    const uint32_t generation = inst.generation + 1;
    inst = SAnimInstance{};
    inst.generation = generation;
    inst.owner      = owner;
    inst.alive      = true;
    aliveCount++;
    return SAnimInstanceHandle{ index, generation };
}

void MAnimationSubsystem::unregisterInstance(SAnimInstanceHandle& handle)
{
    SAnimInstance* inst = getInstance(handle);
    const uint32_t index = handle.index;
    handle = SAnimInstanceHandle{};
    if (!inst)
        return;

    inst->alive = false;
    inst->owner = nullptr;
    aliveCount--;

    // During update() a slot may be in this frame's lists - recycle it after.
    if (updating)
        hasDeadSlots = true;
    else
    {
        inst->local.clear();
        inst->model.clear();
        inst->skinning.clear();
        freeSlots.push_back(index);
    }
}

SAnimInstance* MAnimationSubsystem::getInstance(const SAnimInstanceHandle& handle)
{
    if (handle.isNull() || handle.index >= pool.size())
        return nullptr;
    SAnimInstance* inst = pool[handle.index].get();
    return inst && inst->alive && inst->generation == handle.generation ? inst : nullptr;
}

const SAnimInstance* MAnimationSubsystem::getInstance(const SAnimInstanceHandle& handle) const
{
    return const_cast<MAnimationSubsystem*>(this)->getInstance(handle);
}

void MAnimationSubsystem::compactPool()
{
    for (uint32_t i = 0; i < pool.size(); ++i)
    {
        SAnimInstance* inst = pool[i].get();
        if (!inst || inst->alive || inst->owner != nullptr)
            continue;
        if (std::find(freeSlots.begin(), freeSlots.end(), i) != freeSlots.end())
            continue;
        inst->local.clear();
        inst->model.clear();
        inst->skinning.clear();
        freeSlots.push_back(i);
    }
    hasDeadSlots = false;
}

// ---------------------------------------------------------------------------
// Skinning backend
// ---------------------------------------------------------------------------

void MAnimationSubsystem::setSkinningBackend(ESkinningBackend type)
{
    if (type == backendType && backend)
        return;
    backendType = type;
    if (backend)
    {
        backend->cleanup();
        backend.reset();
    }
    backendReady = false;
}

void MAnimationSubsystem::ensureBackend()
{
    if (backendReady)
        return;

    auto prepareBackend = [](ESkinningBackend type) -> std::unique_ptr<ISkinningBackend>
    {
        if (type == ESkinningBackend::GPU)
            return std::make_unique<MGpuSkinningBackend>();
        return std::make_unique<MCpuSkinningBackend>();
    };

    backend = prepareBackend(backendType);
    if (!backend->init())
    {
        MWARN(STR("MAnimationSubsystem:: ") + backend->getName() + " skinning unavailable, switching to CPU backend");
        backendType = ESkinningBackend::CPU;
        backend = prepareBackend(backendType);
        backend->init();
    }
    MLOG(STR("MAnimationSubsystem:: skinning backend: ") + backend->getName());
    backendReady = true;
}

// ---------------------------------------------------------------------------
// Culling
// ---------------------------------------------------------------------------

void MAnimationSubsystem::buildFrustums()
{
    frustums.clear();
    auto* views = MEngineSubsystemRegistry::getSubsystem<MViewManagement>();
    if (!views)
        return;
    for (auto* camera : views->getCameras())
    {
        if (!camera || !camera->isEnabledInHierarchy())
            continue;
        SFrustum frustum;
        frustum.extractFromVP(camera->getProjectionMatrix(CULLING_RESOLUTION) * camera->getViewMatrix());
        frustums.push_back(frustum);
    }
}

bool MAnimationSubsystem::isVisible(const AABB& bounds) const
{
    if (frustums.empty() || bounds.min == bounds.max)
        return true;   // no camera / no bounds: never cull
    for (const auto& frustum : frustums)
        if (frustum.testAABB(bounds))
            return true;
    return false;
}

// ---------------------------------------------------------------------------
// Pose maths (jobs) - see pose_evaluation.h
// ---------------------------------------------------------------------------

void MAnimationSubsystem::computeModelSpace(SAnimInstance& inst)
{
    MPoseEvaluation::computeModelSpace(inst);
}

void MAnimationSubsystem::evaluatePose(SAnimInstance& inst)
{
    MPoseEvaluation::evaluate(inst);
}

// ---------------------------------------------------------------------------
// Modifiers
// ---------------------------------------------------------------------------

void MAnimationSubsystem::runModifiers(SAnimInstance& inst, float deltaTime)
{
    MSkeletalMeshEntity* owner = inst.owner;
    const SSkeleton& skeleton = *inst.skeleton;

    struct SOrdered { int key; int order; IBoneModifier* modifier; };
    std::vector<SOrdered> modifiers;
    int order = 0;
    for (auto* child : owner->getChildren())
    {
        if (!child || !child->isEnabledInHierarchy()) continue;
        if (auto* modifier = dynamic_cast<IBoneModifier*>(child))
        {
            const int key = modifier->getModifierSortKey(skeleton);
            if (key >= 0)
                modifiers.push_back({ key, order++, modifier });
        }
    }
    if (modifiers.empty())
        return;

    // Parent-first bone order; on the same bone, hierarchy order (the last wins).
    std::stable_sort(modifiers.begin(), modifiers.end(),
                     [](const SOrdered& a, const SOrdered& b) { return a.key < b.key; });

    const SMatrix4 world = owner->getTransformMatrix();
    SPoseEditContext context(inst, skeleton, world);
    for (auto& m : modifiers)
        m.modifier->applyModifier(context, deltaTime);
}

// ---------------------------------------------------------------------------
// The frame
// ---------------------------------------------------------------------------

void MAnimationSubsystem::update(float deltaTime)
{
    if (aliveCount == 0)
        return;

    START_PROFILING_SAMPLE("Animation.Update")
    updating = true;

    const MApplication* app = MApplication::getAppInstance();
    const bool simulating   = app && app->isSimulating();
    const bool advanceTime  = simulating && !(app && app->isPaused());

    buildFrustums();
    evaluated.clear();
    finishedClips.clear();

    // -- 1. Prepare, advance time, decide ---------------------------------------
    for (size_t p = 0; p < pool.size(); ++p)
    {
        SAnimInstance& inst = *pool[p];
        inst.evaluate = inst.skin = false;
        inst.evalCount = 0;
        if (!inst.alive || !inst.owner)
            continue;

        MSkeletalMeshEntity* owner = inst.owner;
        if (!owner->isEnabledInHierarchy())
            continue;

        const SSkeleton* skeleton = owner->animPrepareFrame(inst);
        inst.skeleton = skeleton;
        if (!skeleton || skeleton->getBoneCount() == 0)
            continue;

        if (inst.skeletonId != skeleton->id || static_cast<int>(inst.local.size()) != skeleton->getBoneCount())
        {
            inst.resizeFor(skeleton->getBoneCount());
            inst.skeletonId = skeleton->id;
            inst.hasPose    = false;
        }

        const EAnimationCullingMode mode = owner->getCullingMode();
        inst.visible = isVisible(owner->animGetCullBounds());

        // A pose is needed at least once, culled or not, so the bounds and
        // the skinned buffer start from the right place.
        const bool full = inst.visible || mode == EAnimationCullingMode::AlwaysAnimate || !inst.hasPose;

        if (simulating)
        {
            const bool timeRuns = advanceTime && (full || mode != EAnimationCullingMode::Pause);
            if (timeRuns)
            {
                finishedScratch.clear();
                inst.player.advance(deltaTime, finishedScratch);
                for (auto& name : finishedScratch)
                    finishedClips.push_back({ owner->handle, name });
            }
        }
#if METEOR_EDITOR
        else
        {
            owner->animDriveEditorPreview(inst, deltaTime);
        }
#endif

        inst.evaluate = full || owner->animHasOffscreenSocket();
        inst.skin     = full;
    }

    // Finished clips go out before the poses are built, so a playClip from a
    // listener shows this frame.
    for (const auto& finished : finishedClips)
        if (SAnimInstance* inst = getInstance(finished.handle))
            inst->owner->animDispatchClipFinished(finished.clipName);

    for (size_t p = 0; p < pool.size(); ++p)
    {
        SAnimInstance& inst = *pool[p];
        if (!inst.alive || !inst.owner || !inst.evaluate || !inst.skeleton)
            continue;

        inst.evalCount = 0;
        const SAnimationPlayer& player = inst.player;
        for (int e = 0; e < player.entryCount; ++e)
        {
            const auto& entry = player.entries[e];
            if (!entry.clip || !entry.binding || !entry.binding->isPlayable() || entry.weight <= 0.0f)
                continue;
            auto& eval   = inst.evalEntries[inst.evalCount++];
            eval.clip    = entry.clip;
            eval.binding = entry.binding;
            eval.time    = std::clamp(entry.time, 0.0f, entry.duration);
            eval.weight  = entry.weight;
        }
        evaluated.push_back(&inst);
    }

    // -- 2. Evaluate poses in parallel --------------------------------------------
    // One task per instance - a pose is already coarse work - and parallel
    // from two instances up. Runs serially without a job system.
    MParallel::forEach(std::span(evaluated),
                       [](SAnimInstance* inst) { evaluatePose(*inst); },
                       POSE_JOB_CHUNK_SIZE, POSE_JOB_THRESHOLD);

    // -- 3. Bone modifiers, then onPostAnimate ------------------------------------
    runningModifiers = true;
    for (auto* inst : evaluated)
    {
        if (!inst->alive || !inst->owner) continue;
        runModifiers(*inst, deltaTime);
        if (!inst->alive || !inst->owner) continue;

        inst->poseEditable = true;
        inst->owner->animRunPostAnimate(deltaTime);
        inst->poseEditable = false;
    }
    runningModifiers = false;

    // -- 4. Re-walk what changed --------------------------------------------------
    for (auto* inst : evaluated)
    {
        if (!inst->alive || !inst->owner) continue;
        if (inst->poseDirty)
            computeModelSpace(*inst);
        inst->owner->animOnPoseUpdated(*inst);
    }

    // -- 5. Sockets ----------------------------------------------------------------
    for (auto* inst : evaluated)
        if (inst->alive && inst->owner)
            inst->owner->animUpdateSockets(*inst);

    // -- 6. Upload and skin ------------------------------------------------------------
    boneMatrices.clear();
    skinJobs.clear();
    for (auto* inst : evaluated)
    {
        if (!inst->alive || !inst->owner || !inst->skin) continue;
        inst->boneOffset = static_cast<uint32_t>(boneMatrices.size());
        boneMatrices.insert(boneMatrices.end(), inst->skinning.begin(), inst->skinning.end());
        inst->owner->animCollectSkinJobs(inst->boneOffset, skinJobs);
    }
    if (!skinJobs.empty())
    {
        ensureBackend();
        START_PROFILING_SAMPLE("Animation.Skinning")
        backend->skin(boneMatrices, skinJobs);
        STOP_PROFILING_SAMPLE("Animation.Skinning")
    }

    updating = false;
    if (hasDeadSlots)
        compactPool();
    STOP_PROFILING_SAMPLE("Animation.Update")
}
