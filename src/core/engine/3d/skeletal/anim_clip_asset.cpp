#include "anim_clip_asset.h"

#include "anim_clip.h"
#include "core/utils/logger.h"
#include "skeletal_model_loader.h"

MAnimClipAsset::MAnimClipAsset(const SString& assetPath, const SModelImportSettings& settings)
    : MAsset(assetPath)
    , importSettings(settings)
{
    importSettings.importAs = EModelImportAs::AnimationOnly;
    load();
}

MAnimClipAsset::~MAnimClipAsset()
{
    for (auto* clip : clips) delete clip;
}

void MAnimClipAsset::load()
{
    SSkeletalModelData data;
    SString error;
    if (!MSkeletalModelLoader::load(path, importSettings, MSkeletalModelLoader::EContent::ClipsOnly, data, error))
    {
        MERROR(STR("MAnimClipAsset:: failed to import ") + path + " - " + error);
        for (auto* clip : data.clips) delete clip;
        valid = !clips.empty();
        return;
    }

    for (auto* clip : clips) delete clip;
    clips      = std::move(data.clips);
    generation = nextAnimationDataId();
    valid      = true;
}

std::vector<SString> MAnimClipAsset::getClipNames() const
{
    std::vector<SString> names;
    names.reserve(clips.size());
    for (const auto* clip : clips)
        names.push_back(clip->getName());
    return names;
}
