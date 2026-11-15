#include "bone_binding.h"
#include "animation_helpers.h"

#include <unordered_map>

#include "core/engine/3d/skeletal/anim_clip.h"
#include "core/engine/3d/skeletal/skeleton.h"

namespace
{
    struct SBindingKey
    {
        uint64_t clipId;
        uint64_t skeletonId;
        bool operator==(const SBindingKey& o) const { return clipId == o.clipId && skeletonId == o.skeletonId; }
    };

    struct SBindingKeyHash
    {
        size_t operator()(const SBindingKey& k) const
        {
            return std::hash<uint64_t>()(MAnimationHelpers::hashCombine(k.clipId, k.skeletonId));
        }
    };

    // Node-based: references to values survive rehashing.
    std::unordered_map<SBindingKey, SClipBinding, SBindingKeyHash>& cache()
    {
        static std::unordered_map<SBindingKey, SClipBinding, SBindingKeyHash> bindings;
        return bindings;
    }

    SClipBinding build(const MAnimClip& clip, const SSkeleton& skeleton)
    {
        SClipBinding binding;
        const int boneCount = skeleton.getBoneCount();
        std::vector<bool> boneHasTrack(boneCount, false);

        binding.trackToBone.resize(clip.getTrackCount(), -1);
        for (int t = 0; t < clip.getTrackCount(); ++t)
        {
            const int bone = MBoneBinding::resolveBoneIndex(skeleton, clip.getTrack(t).boneName);
            binding.trackToBone[t] = bone;
            if (bone >= 0)
                boneHasTrack[bone] = true;
        }

        // Count over deform bones: armature, mesh and helper nodes never have
        // tracks and would make every clip look partial.
        bool anyDeform = false;
        for (const auto& bone : skeleton.bones) anyDeform |= bone.isDeformBone;
        for (int b = 0; b < boneCount; ++b)
        {
            if (anyDeform && !skeleton.bones[b].isDeformBone) continue;
            ++binding.totalBones;
            if (boneHasTrack[b]) ++binding.matchedBones;
        }

        binding.rootHasTrack = skeleton.rootBone >= 0 && skeleton.rootBone < boneCount && boneHasTrack[skeleton.rootBone];

        if (binding.matchedBones == 0 || !binding.rootHasTrack)
            binding.status = EClipStatus::Unavailable;
        else if (binding.matchedBones < binding.totalBones)
            binding.status = EClipStatus::Warning;
        else
            binding.status = EClipStatus::Available;
        return binding;
    }
}

namespace MBoneBinding
{
    int resolveBoneIndex(const SSkeleton& skeleton, const SString& trackName)
    {
        return skeleton.findBone(trackName);
    }

    const SClipBinding& getBinding(const MAnimClip& clip, const SSkeleton& skeleton)
    {
        const SBindingKey key{ clip.getId(), skeleton.id };
        auto& bindings = cache();
        auto it = bindings.find(key);
        if (it == bindings.end())
            it = bindings.emplace(key, build(clip, skeleton)).first;
        return it->second;
    }

    SString describeUnavailable(const SClipBinding& binding, const SSkeleton& skeleton, const SString& skeletonName)
    {
        if (binding.isPlayable())
            return {};
        if (binding.matchedBones == 0)
            return SString::format("no bones match {0} skeleton", skeletonName);
        const SString rootName = skeleton.rootBone >= 0 && skeleton.rootBone < skeleton.getBoneCount()
            ? skeleton.bones[skeleton.rootBone].name : SString("(none)");
        return SString::format("root bone '{0}' of {1} skeleton has no track", rootName, skeletonName);
    }
}
