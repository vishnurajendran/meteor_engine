#include "pose_evaluation.h"

#include <algorithm>
#include <limits>

#include "core/engine/3d/skeletal/anim_clip.h"
#include "core/engine/3d/skeletal/skeleton.h"
#include "bone_binding.h"

void MPoseEvaluation::computeModelSpace(SAnimInstance& inst)
{
    const SSkeleton& skeleton = *inst.skeleton;
    const int boneCount = static_cast<int>(inst.local.size());

    SVector3 deformMin(std::numeric_limits<float>::max()), deformMax(std::numeric_limits<float>::lowest());
    SVector3 allMin = deformMin, allMax = deformMax;
    bool anyDeform = false;

    for (int i = 0; i < boneCount; ++i)
    {
        const SBone& bone = skeleton.bones[i];
        const SMatrix4 local = inst.local[i].toMatrix();
        inst.model[i]    = bone.parent < 0 ? skeleton.rootOffset * local : inst.model[bone.parent] * local;
        inst.skinning[i] = inst.model[i] * bone.inverseBind;

        const SVector3 p(inst.model[i][3]);
        allMin = glm::min(allMin, p);
        allMax = glm::max(allMax, p);
        if (bone.isDeformBone)
        {
            anyDeform = true;
            deformMin = glm::min(deformMin, p);
            deformMax = glm::max(deformMax, p);
        }
    }

    if (boneCount > 0)
    {
        inst.jointMin = anyDeform ? deformMin : allMin;
        inst.jointMax = anyDeform ? deformMax : allMax;
    }
    inst.hasPose   = true;
    inst.poseDirty = false;
}

void MPoseEvaluation::evaluate(SAnimInstance& inst)
{
    const SSkeleton& skeleton = *inst.skeleton;
    const int boneCount = static_cast<int>(inst.local.size());

    if (inst.evalCount == 0)
    {
        for (int i = 0; i < boneCount; ++i)
            inst.local[i] = skeleton.bones[i].bindPose;
        computeModelSpace(inst);
        return;
    }

    std::fill(inst.accPosition.begin(), inst.accPosition.end(), SVector3(0.0f));
    std::fill(inst.accRotation.begin(), inst.accRotation.end(), SVector4(0.0f));
    std::fill(inst.accScale.begin(),    inst.accScale.end(),    SVector3(0.0f));
    std::fill(inst.accWeight.begin(),   inst.accWeight.end(),   0.0f);

    for (int e = 0; e < inst.evalCount; ++e)
    {
        const auto& entry = inst.evalEntries[e];
        const float w = entry.weight;
        const auto& trackToBone = entry.binding->trackToBone;
        const int trackCount = std::min(entry.clip->getTrackCount(), static_cast<int>(trackToBone.size()));

        for (int t = 0; t < trackCount; ++t)
        {
            const int bone = trackToBone[t];
            if (bone < 0 || bone >= boneCount) continue;   // no such bone: ignored

            const STransform sample = entry.clip->sampleTrack(t, entry.time);
            SQuaternion q = sample.rotation;
            // Keep every contribution on the bind rotation's hemisphere so the
            // normalized sum blends the short way.
            if (glm::dot(q, skeleton.bones[bone].bindPose.rotation) < 0.0f)
                q = -q;

            inst.accPosition[bone] += sample.position * w;
            inst.accScale[bone]    += sample.scale * w;
            inst.accRotation[bone] += SVector4(q.x, q.y, q.z, q.w) * w;
            inst.accWeight[bone]   += w;
        }
    }

    for (int i = 0; i < boneCount; ++i)
    {
        const STransform& bind = skeleton.bones[i].bindPose;
        // Bones with no track (or partly covered) take the rest from bind pose.
        const float rest = 1.0f - inst.accWeight[i];
        SVector3 position = inst.accPosition[i];
        SVector3 scale    = inst.accScale[i];
        SVector4 rotation = inst.accRotation[i];
        if (rest > 1e-4f)
        {
            position += bind.position * rest;
            scale    += bind.scale * rest;
            rotation += SVector4(bind.rotation.x, bind.rotation.y, bind.rotation.z, bind.rotation.w) * rest;
        }

        STransform& out = inst.local[i];
        out.position = position;
        out.scale    = scale;
        const float len = glm::length(rotation);
        out.rotation = len > 1e-8f ? SQuaternion(rotation.w / len, rotation.x / len, rotation.y / len, rotation.z / len)
                                   : bind.rotation;
    }

    computeModelSpace(inst);
}
