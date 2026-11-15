#include "bone_control_entity.h"

#include <algorithm>

#include "core/engine/animation/aim_math.h"
#include "core/engine/animation/pose.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/utils/transform.h"
#include "skeletal_mesh_entity.h"
#include "skeleton.h"

IMPLEMENT_SPATIAL_CLASS(MBoneControlEntity)

namespace
{
    // Rotation part of a model matrix that may carry scale (e.g. an FBX root).
    SQuaternion rotationOf(const SMatrix4& m)
    {
        return STransform::fromMatrix(m).rotation;
    }
}

MBoneControlEntity::MBoneControlEntity()
{
    name = "BoneControl";
}

MSkeletalMeshEntity* MBoneControlEntity::getSkeletalMeshParent() const
{
    return dynamic_cast<MSkeletalMeshEntity*>(parent);
}

int MBoneControlEntity::resolveBone(const SSkeleton& skeleton) const
{
    if (cachedSkeletonId != skeleton.id || cachedBoneName != boneName.get())
    {
        cachedBone       = skeleton.findBone(boneName.get());
        cachedSkeletonId = skeleton.id;
        cachedBoneName   = boneName.get();
    }
    return cachedBone;
}

bool MBoneControlEntity::isBoneValid() const
{
    const auto* mesh = getSkeletalMeshParent();
    const SSkeleton* skeleton = mesh ? mesh->getSkeleton() : nullptr;
    return skeleton && resolveBone(*skeleton) >= 0;
}

// -- Look ---------------------------------------------------------------------------

void MBoneControlEntity::lookAt(const SVector3& worldTarget)
{
    aimOnce      = true;
    aimOncePoint = worldTarget;
}

void MBoneControlEntity::setLookTarget(MSpatialEntity* target)
{
    lookTarget = target;
    lookActive = target != nullptr;
    if (!lookActive)
        return;
    if (lookBlend <= 0.0f)
        hasCurrentLook = false;   // start the turn from the animated pose
}

void MBoneControlEntity::clearLookTarget()
{
    lookTarget = nullptr;
    lookActive = false;
}

bool MBoneControlEntity::hasLookTarget() const
{
    return lookActive && lookTarget && !lookTarget->hasExited();
}

// -- Setup ----------------------------------------------------------------------------

bool MBoneControlEntity::snapToBone()
{
    auto* mesh = getSkeletalMeshParent();
    const SSkeleton* skeleton = mesh ? mesh->getSkeleton() : nullptr;
    if (!skeleton)
        return false;
    const int bone = resolveBone(*skeleton);
    if (bone < 0)
        return false;

    std::vector<SMatrix4> bindModel;
    skeleton->computeBindModelTransforms(bindModel);
    const STransform bind = STransform::fromMatrix(bindModel[bone]);

    relativePosition = bind.position;
    relativeRotation = bind.rotation;
    updateTransforms();

    snapRotation = bind.rotation;
    // The character's forward (+Z) and up (+Y) at bind pose, in the bone's frame.
    const SQuaternion toBone = glm::inverse(bind.rotation);
    aimAxis = glm::normalize(toBone * SVector3(0.0f, 0.0f, 1.0f));
    upAxis  = glm::normalize(toBone * SVector3(0.0f, 1.0f, 0.0f));
    hasCurrentLook = false;
    return true;
}

// -- Gizmo ------------------------------------------------------------------------------

void MBoneControlEntity::onDrawGizmo(SVector2 res)
{
    MSpatialEntity::onDrawGizmo(res);
#if METEOR_EDITOR
    // The control sits on the bone, so its world rotation maps the bone-frame
    // axes into the world.
    const SVector3    origin = getWorldPosition();
    const SQuaternion rot    = getWorldRotation();
    constexpr float LENGTH = 0.25f;
    if (glm::length(aimAxis.get()) > 1e-4f)
        MGizmos::drawRay(origin, glm::normalize(rot * aimAxis.get()), LENGTH, SColor::skyBlue(), 2.0f);
    if (glm::length(upAxis.get()) > 1e-4f)
        MGizmos::drawRay(origin, glm::normalize(rot * upAxis.get()), LENGTH * 0.6f, SColor::yellow(), 2.0f);
#endif
}

// -- IBoneModifier ------------------------------------------------------------------------

int MBoneControlEntity::getModifierSortKey(const SSkeleton& skeleton) const
{
    if (weight.get() <= 0.0f && !aimOnce && !hasLookTarget() && lookBlend <= 0.0f)
        return -1;
    return resolveBone(skeleton);
}

void MBoneControlEntity::applyModifier(SPoseEditContext& pose, float deltaTime)
{
    const int bone = resolveBone(pose.getSkeleton());
    if (bone < 0 || bone >= pose.getBoneCount())
        return;

    if (lookActive && (!lookTarget || lookTarget->hasExited()))
        clearLookTarget();   // the target was destroyed

    const STransform  animatedLocal    = pose.getLocal(bone);
    const SQuaternion parentRot        = rotationOf(pose.getParentModel(bone));
    const SQuaternion animatedModelRot = glm::normalize(parentRot * animatedLocal.rotation);

    // Look fade.
    const float fadeTarget = (lookActive || aimOnce) ? 1.0f : 0.0f;
    if (aimOnce)
        lookBlend = 1.0f;   // a one-off aim applies in full this frame
    else if (blendTime.get() <= 0.0f)
        lookBlend = fadeTarget;
    else
    {
        const float step = deltaTime / blendTime.get();
        lookBlend = fadeTarget > lookBlend ? std::min(fadeTarget, lookBlend + step)
                                           : std::max(fadeTarget, lookBlend - step);
    }

    // The control's own rotation: it is a child of the mesh entity, so its
    // relative rotation is already in the skeleton's model space.
    SQuaternion target = relativeRotation;

    if (lookBlend > 0.0f)
    {
        if (lookActive || aimOnce)
        {
            const SVector3 worldPoint = aimOnce ? aimOncePoint : lookTarget->getWorldPosition();
            const SVector3 modelPoint = SVector3(glm::inverse(pose.getEntityWorld()) * SVector4(worldPoint, 1.0f));
            const SVector3 bonePos    = SVector3(pose.getModel(bone)[3]);
            const SVector3 direction  = modelPoint - bonePos;

            if (glm::length(direction) > 1e-5f)
            {
                const SQuaternion aim = MAimMath::aimRotation(aimAxis.get(), upAxis.get(), direction,
                                                              SVector3(0.0f, 1.0f, 0.0f));
                // Additive looks are measured from the snapped pose, so the
                // look rotation is turned into a bone rotation the same way.
                if (!hasCurrentLook)
                {
                    currentLook    = mode.get() == EBoneControlMode::Additive ? snapRotation.get() : animatedModelRot;
                    hasCurrentLook = true;
                }
                currentLook = turnSpeed.get() > 0.0f
                    ? MAimMath::rotateTowards(currentLook, aim, glm::radians(turnSpeed.get()) * deltaTime)
                    : aim;
            }
        }
        if (hasCurrentLook)
            target = glm::normalize(glm::slerp(target, currentLook, lookBlend));
    }
    else
    {
        hasCurrentLook = false;
    }

    SQuaternion newModelRot;
    if (mode.get() == EBoneControlMode::Override)
        newModelRot = target;
    else
    {
        // The control's change since it was snapped, on top of the animation.
        const SQuaternion delta = target * glm::inverse(snapRotation.get());
        newModelRot = glm::normalize(delta * animatedModelRot);
    }

    newModelRot = glm::normalize(glm::slerp(animatedModelRot, newModelRot, std::clamp(weight.get(), 0.0f, 1.0f)));
    newModelRot = MAimMath::clampAngle(animatedModelRot, newModelRot, glm::radians(std::max(0.0f, maxAngle.get())));

    STransform newLocal = animatedLocal;
    newLocal.rotation   = glm::normalize(glm::inverse(parentRot) * newModelRot);
    pose.setLocal(bone, newLocal);

    aimOnce = false;
}
