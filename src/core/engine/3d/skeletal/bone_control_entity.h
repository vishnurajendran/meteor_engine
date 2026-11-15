//
// bone_control_entity.h
//
// MBoneControlEntity - an entity drives a bone. Must be a direct child of an
// MSkeletalMeshEntity. The control's own rotation (relative to the mesh
// entity, i.e. in the skeleton's model space) is copied onto the bone, or the
// bone aims at a target (lookAt / setLookTarget).
//
//   Override   the control's rotation replaces the animated rotation
//   Additive   the control's change since it was snapped to the bone is added
//              on top of the animation - with maxAngle ~70 degrees a head look
//              keeps the idle motion and holds at the limit when the target
//              goes behind
//
// weight blends between the animated rotation and the control's, maxAngle
// clamps how far the bone turns from its animated rotation (0 = no limit),
// turnSpeed (degrees per second, 0 = instant) smooths look turns, blendTime
// fades the look in and out on setLookTarget / clearLookTarget.
//
// Runs as the first IBoneModifier, in parent-first bone order: the bone's new
// local rotation is inverse(parentBoneModelRot) x targetModelRot, using the
// parent's rotation after any controls on ancestor bones. Two controls on one
// bone: the last in hierarchy order wins. Works in edit mode too.
//
#pragma once
#ifndef BONE_CONTROL_ENTITY_H
#define BONE_CONTROL_ENTITY_H

#include "core/engine/animation/bone_modifier.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/object/objectpointer.h"

class MSkeletalMeshEntity;
struct SSkeleton;

SCRIPT_BIND_ENUM()
enum class EBoneControlMode : int
{
    Override = 0,
    Additive = 1,
};

SCRIPT_BIND_CLASS()
class MBoneControlEntity : public MSpatialEntity, public IBoneModifier
{
    DEFINE_SPATIAL_CLASS(MBoneControlEntity)

    DECLARE_FIELD(boneName,  SString, "")
    DECLARE_FIELD(mode,      EBoneControlMode, EBoneControlMode::Additive)
    DECLARE_FIELD(weight,    float, 1.0f)       // 0..1
    DECLARE_FIELD(maxAngle,  float, 0.0f)       // degrees, 0 = no limit
    // In the bone's frame. Set on snap to the character's forward (+Z) and up
    // (+Y) at bind pose.
    DECLARE_FIELD(aimAxis,   SVector3, SVector3(0.0f, 0.0f, 1.0f))
    DECLARE_FIELD(upAxis,    SVector3, SVector3(0.0f, 1.0f, 0.0f))
    DECLARE_FIELD(turnSpeed, float, 0.0f)       // degrees per second, 0 = instant
    DECLARE_FIELD(blendTime, float, 0.2f)       // seconds, look fade in / out
    // The control's rotation when it was snapped to the bone - Additive mode
    // adds the change from here.
    DECLARE_FIELD(snapRotation, SQuaternion, SQuaternion(1.0f, 0.0f, 0.0f, 0.0f))

public:
    MBoneControlEntity();

    // -- Look ---------------------------------------------------------------------------
    // Aim once, this frame.
    SCRIPT_BIND_FUNC()
    void lookAt(const SVector3& worldTarget);
    // Keep tracking an entity (fades in over blendTime).
    SCRIPT_BIND_FUNC()
    void setLookTarget(MSpatialEntity* target);
    // Fade back to the animation.
    SCRIPT_BIND_FUNC()
    void clearLookTarget();
    SCRIPT_BIND_FUNC()
    bool hasLookTarget() const;

    // -- Setup --------------------------------------------------------------------------
    SCRIPT_BIND_FUNC()
    void setBoneName(const SString& name) { boneName = name; cachedSkeletonId = 0; }
    SCRIPT_BIND_FUNC()
    SString getBoneName() const { return boneName.get(); }

    // Moves the control onto the bone's bind-pose transform, records it as
    // the Additive start, and points aimAxis / upAxis at the character's
    // forward and up in the bone's frame. The editor calls it when a bone is
    // picked. False if there is no such bone.
    SCRIPT_BIND_FUNC()
    bool snapToBone();

    SCRIPT_BIND_FUNC()
    MSkeletalMeshEntity* getSkeletalMeshParent() const;
    SCRIPT_BIND_FUNC()
    bool isBoneValid() const;

    // Editor: the aim (blue) and up (yellow) axes.
    void onDrawGizmo(SVector2 renderResolution) override;

    // IBoneModifier
    [[nodiscard]] int getModifierSortKey(const SSkeleton& skeleton) const override;
    void applyModifier(SPoseEditContext& pose, float deltaTime) override;

private:
    int resolveBone(const SSkeleton& skeleton) const;

    mutable int      cachedBone       = -1;
    mutable uint64_t cachedSkeletonId = 0;
    mutable SString  cachedBoneName;

    MObjectPtr<MSpatialEntity> lookTarget;
    bool        lookActive   = false;   // a target is set
    bool        aimOnce      = false;   // lookAt() this frame
    SVector3    aimOncePoint = SVector3(0.0f);
    float       lookBlend    = 0.0f;    // 0 = own rotation, 1 = look rotation
    SQuaternion currentLook  = SQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
    bool        hasCurrentLook = false;
};

#endif // BONE_CONTROL_ENTITY_H
