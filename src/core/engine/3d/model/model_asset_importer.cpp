#include "model_asset_importer.h"

#include <algorithm>
#include <array>

#include "default_engine_icon_paths.h"
#include "core/engine/3d/skeletal/anim_clip_asset.h"
#include "core/engine/3d/skeletal/skeletal_mesh_asset.h"
#include "core/engine/3d/skeletal/skeletal_model_loader.h"
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/memory/mnew.h"
#include "core/utils/logger.h"
#include "model_import_settings.h"

bool MModelAssetImporter::registered = []() {
    addImporter(mnew<MModelAssetImporter>());
    return true;
}();

bool MModelAssetImporter::canImport(SString fileExtension)
{
    static const std::array<SString, 4> supported = { "gltf", "glb", "obj", "fbx" };
    return std::find(supported.begin(), supported.end(), fileExtension) != supported.end();
}

SString MModelAssetImporter::getIconPath() const
{
    return SEngineAssetIconPaths::HIGHRES_TEX_ASSET_STATICMESH;
}

MAsset* MModelAssetImporter::importAsset(SString path, const pugi::xml_document& metaData)
{
    const auto settingsNode = metaData.child("asset_id").child("meshImport");

    SModelImportSettings settings;
    settings.load(settingsNode);

    if (settings.importAs == EModelImportAs::Auto)
    {
        settings.importAs = MSkeletalModelLoader::detect(path);
        // Write the result back so later loads skip detection and the
        // inspector shows it. Read-only sources (published games) just
        // detect again next time.
        if (settings.saveToMeta(path))
            MLOG(SString::format("MModelAssetImporter:: {0} detected as {1}", path,
                                 SString(SModelImportSettings::toString(settings.importAs))));
    }

    MAsset* asset = nullptr;
    switch (settings.importAs)
    {
        case EModelImportAs::SkeletalMesh:
            asset = mnew<MSkeletalMeshAsset>(path, settings);
            break;
        case EModelImportAs::AnimationOnly:
            asset = mnew<MAnimClipAsset>(path, settings);
            break;
        case EModelImportAs::StaticMesh:
        case EModelImportAs::Auto:
        default:
            // A rigged file imported this way loads in bind pose, bones ignored.
            asset = settingsNode ? mnew<MStaticMeshAsset>(path, settingsNode) : mnew<MStaticMeshAsset>(path);
            if (auto* staticAsset = dynamic_cast<MStaticMeshAsset*>(asset))
                staticAsset->getImportSettings().importAs = EModelImportAs::StaticMesh;
            break;
    }

    if (!asset->isValid())
    {
        MERROR(SString::format("MModelAssetImporter::importAsset(): could not import {0} as {1}", path,
                               SString(SModelImportSettings::toString(settings.importAs))));
        delete asset;
        return nullptr;
    }
    return asset;
}
