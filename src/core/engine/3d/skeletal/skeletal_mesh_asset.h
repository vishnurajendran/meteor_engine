//
// skeletal_mesh_asset.h
//
// MSkeletalMeshAsset - a model file imported with importAs = SkeletalMesh:
// its skinned sub-meshes, its skeleton and its embedded clips.
//
#pragma once
#ifndef SKELETAL_MESH_ASSET_H
#define SKELETAL_MESH_ASSET_H

#include <vector>

#include "anim_clip_source.h"
#include "core/engine/3d/model/model_import_settings.h"
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/utils/glmhelper.h"
#include "skeleton.h"

class MSkeletalMesh;
class MAnimClip;
namespace pugi { class xml_node; }

SCRIPT_BIND_CLASS()
class MSkeletalMeshAsset : public MAsset, public IAnimationClipSource
{
    DEFINE_OBJECT_SUBCLASS(MSkeletalMeshAsset)
public:
    MSkeletalMeshAsset(const SString& path, const SModelImportSettings& settings);
    ~MSkeletalMeshAsset() override;

    [[nodiscard]] const std::vector<MSkeletalMesh*>& getMeshes() const { return meshes; }
    [[nodiscard]] const SSkeleton& getSkeleton() const { return skeleton; }

    SCRIPT_BIND_FUNC()
    int getSubMeshCount() const { return static_cast<int>(meshes.size()); }
    SCRIPT_BIND_FUNC()
    int getBoneCount() const { return skeleton.getBoneCount(); }
    SCRIPT_BIND_FUNC()
    std::vector<SString> getClipNames() const;

    // IAnimationClipSource
    const std::vector<MAnimClip*>& getClips() const override { return clips; }
    [[nodiscard]] uint64_t getClipGeneration() const override { return generation; }

    // Changes on every successful (re)load - entities rebuild their GPU
    // buffers and clip tables when it does.
    [[nodiscard]] uint64_t getGeneration() const { return generation; }

    // Bind pose, scaled, pivot applied.
    [[nodiscard]] SVector3 getBoundsMin() const { return boundsMin; }
    [[nodiscard]] SVector3 getBoundsMax() const { return boundsMax; }
    [[nodiscard]] SVector3 getJointBoundsPadding() const { return jointBoundsPadding; }

    // -- Import settings (take effect on reload) ---------------------------------
    const SModelImportSettings& getImportSettings() const { return importSettings; }
    SModelImportSettings&       getImportSettings()       { return importSettings; }
    bool saveImportSettings() { return importSettings.saveToMeta(path); }

    bool requestReload() override { load(); return valid; }

private:
    // On failure the previously loaded data is kept.
    void load();
    void clear();

    SModelImportSettings       importSettings;
    SSkeleton                  skeleton;
    std::vector<MSkeletalMesh*> meshes;
    std::vector<MAnimClip*>    clips;
    SVector3                   boundsMin = SVector3(0.0f);
    SVector3                   boundsMax = SVector3(0.0f);
    SVector3                   jointBoundsPadding = SVector3(0.0f);
    uint64_t                   generation = 0;
};

#endif // SKELETAL_MESH_ASSET_H
