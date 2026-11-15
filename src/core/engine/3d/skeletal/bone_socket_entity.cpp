#include "bone_socket_entity.h"

#include "core/utils/transform.h"
#include "skeletal_mesh_entity.h"
#include "skeleton.h"

IMPLEMENT_SPATIAL_CLASS(MBoneSocketEntity)

MBoneSocketEntity::MBoneSocketEntity()
{
    name = "BoneSocket";
}

void MBoneSocketEntity::setOffset(const SVector3& position, const SQuaternion& rotation, const SVector3& scale)
{
    offsetPosition = position;
    offsetRotation = rotation;
    offsetScale    = scale;
}

MSkeletalMeshEntity* MBoneSocketEntity::getSkeletalMeshParent() const
{
    return dynamic_cast<MSkeletalMeshEntity*>(parent);
}

bool MBoneSocketEntity::isBoneValid() const
{
    const auto* mesh = getSkeletalMeshParent();
    const SSkeleton* skeleton = mesh ? mesh->getSkeleton() : nullptr;
    return skeleton && skeleton->findBone(boneName.get()) >= 0;
}

void MBoneSocketEntity::applyBonePose(const SSkeleton& skeleton, const std::vector<SMatrix4>& boneModel)
{
    if (cachedSkeletonId != skeleton.id || cachedBoneName != boneName.get())
    {
        cachedBone       = skeleton.findBone(boneName.get());
        cachedSkeletonId = skeleton.id;
        cachedBoneName   = boneName.get();
    }
    if (cachedBone < 0 || cachedBone >= static_cast<int>(boneModel.size()))
        return;

    STransform offset;
    offset.position = offsetPosition.get();
    offset.rotation = offsetRotation.get();
    offset.scale    = offsetScale.get();

    const STransform target = STransform::fromMatrix(boneModel[cachedBone] * offset.toMatrix());

    // Follow flags: parts that are off keep the socket's own local value.
    if (followPosition.get()) relativePosition = target.position;
    if (followRotation.get()) relativeRotation = target.rotation;
    if (followScale.get())    relativeScale    = target.scale;
    updateTransforms();   // children follow this frame
}
