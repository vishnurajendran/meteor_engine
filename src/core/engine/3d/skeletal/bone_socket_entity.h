//
// bone_socket_entity.h
//
// MBoneSocketEntity - a bone drives an entity. Must be a direct child of an
// MSkeletalMeshEntity; anything parented to the socket (a sword, a hitbox, a
// muzzle point) follows the bone.
//
// The socket's local transform is bone_model x offset. Its parent is the mesh
// entity, so no world-space maths is needed. The follow flags pick which
// parts of that transform are applied; parts whose flag is off keep the
// socket's own local value.
//
// Updated by MAnimationSubsystem after bone modifiers and onPostAnimate,
// before onLateTick - so a hat on a turned head follows it the same frame.
// Works in edit mode too (bind pose or the preview clip), so offsets can be
// tuned without pressing play.
//
#pragma once
#ifndef BONE_SOCKET_ENTITY_H
#define BONE_SOCKET_ENTITY_H

#include <vector>

#include "core/engine/entities/spatial/spatial.h"

class MSkeletalMeshEntity;
struct SSkeleton;

SCRIPT_BIND_CLASS()
class MBoneSocketEntity : public MSpatialEntity
{
    DEFINE_SPATIAL_CLASS(MBoneSocketEntity)

    DECLARE_FIELD(boneName,       SString, "")    // saved by name, so it survives reimports
    DECLARE_FIELD(followPosition, bool, true)
    DECLARE_FIELD(followRotation, bool, true)
    DECLARE_FIELD(followScale,    bool, false)
    // Applied after the bone, e.g. to seat a sword in the palm.
    DECLARE_FIELD(offsetPosition, SVector3, SVector3(0.0f))
    DECLARE_FIELD(offsetRotation, SQuaternion, SQuaternion(1.0f, 0.0f, 0.0f, 0.0f))
    DECLARE_FIELD(offsetScale,    SVector3, SVector3(1.0f))
    // Forces bone evaluation (not skinning) while the character is culled,
    // for gameplay such as hitboxes or muzzle points.
    DECLARE_FIELD(keepUpdatedOffscreen, bool, false)

public:
    MBoneSocketEntity();

    SCRIPT_BIND_FUNC()
    void setBoneName(const SString& name) { boneName = name; cachedSkeletonId = 0; }
    SCRIPT_BIND_FUNC()
    SString getBoneName() const { return boneName.get(); }

    SCRIPT_BIND_FUNC()
    void setOffset(const SVector3& position, const SQuaternion& rotation, const SVector3& scale);

    // The parent, if it is a skeletal mesh entity (the socket does nothing otherwise).
    SCRIPT_BIND_FUNC()
    MSkeletalMeshEntity* getSkeletalMeshParent() const;

    // False when the parent isn't a skeletal mesh or has no bone of that name.
    SCRIPT_BIND_FUNC()
    bool isBoneValid() const;

    // Called by MSkeletalMeshEntity during the animation update.
    void applyBonePose(const SSkeleton& skeleton, const std::vector<SMatrix4>& boneModel);

private:
    int      cachedBone       = -1;
    uint64_t cachedSkeletonId = 0;
    SString  cachedBoneName;
};

#endif // BONE_SOCKET_ENTITY_H
