#include "skeletal_mesh_asset.h"

#include "anim_clip.h"
#include "core/memory/mnew.h"
#include "core/utils/logger.h"
#include "skeletal_mesh.h"
#include "skeletal_model_loader.h"

MSkeletalMeshAsset::MSkeletalMeshAsset(const SString& assetPath, const SModelImportSettings& settings)
    : MAsset(assetPath)
    , importSettings(settings)
{
    importSettings.importAs = EModelImportAs::SkeletalMesh;
    load();
}

MSkeletalMeshAsset::~MSkeletalMeshAsset()
{
    clear();
}

void MSkeletalMeshAsset::clear()
{
    for (auto* mesh : meshes) delete mesh;
    for (auto* clip : clips)  delete clip;
    meshes.clear();
    clips.clear();
}

void MSkeletalMeshAsset::load()
{
    SSkeletalModelData data;
    SString error;
    if (!MSkeletalModelLoader::load(path, importSettings, MSkeletalModelLoader::EContent::MeshesAndClips, data, error))
    {
        MERROR(STR("MSkeletalMeshAsset:: failed to import ") + path + " - " + error);
        for (auto* clip : data.clips) delete clip;
        valid = !meshes.empty();
        return;
    }

    clear();
    for (auto& sub : data.subMeshes)
        meshes.push_back(mnew<MSkeletalMesh>(std::move(sub.vertices), std::move(sub.offsets),
                                             std::move(sub.influences), std::move(sub.indices)));
    clips              = std::move(data.clips);
    skeleton           = std::move(data.skeleton);
    skeleton.id        = nextAnimationDataId();   // a fresh id: cached bone tables must not match the old one
    skeleton.rebuildLookup();
    boundsMin          = data.boundsMin;
    boundsMax          = data.boundsMax;
    jointBoundsPadding = data.jointBoundsPadding;
    generation         = nextAnimationDataId();
    valid              = true;

    MVERBOSE(SString::format("MSkeletalMeshAsset:: {0} - {1} sub-meshes, {2} bones, {3} clips", path,
                             SString::fromInt((int)meshes.size()), SString::fromInt(skeleton.getBoneCount()),
                             SString::fromInt((int)clips.size())));
}

std::vector<SString> MSkeletalMeshAsset::getClipNames() const
{
    std::vector<SString> names;
    names.reserve(clips.size());
    for (const auto* clip : clips)
        names.push_back(clip->getName());
    return names;
}
