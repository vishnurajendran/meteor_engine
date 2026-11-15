//
// model_asset_importer.h
//
// The one importer for model files (gltf, glb, fbx, obj). Replaces the old
// static-mesh-only importer's extension check. It reads importAs from the
// .meta file and creates the matching asset class:
//
//   StaticMesh     -> MStaticMeshAsset
//   SkeletalMesh   -> MSkeletalMeshAsset
//   AnimationOnly  -> MAnimClipAsset
//   Auto           -> detected once (see MSkeletalModelLoader::detect), then
//                     written back to the meta so later loads skip detection
//
// Changing importAs changes the C++ class, so the asset has to be destroyed
// and recreated (MAssetManager::reimportAsset) - a plain reload can't do it.
//
#pragma once
#ifndef MODEL_ASSET_IMPORTER_H
#define MODEL_ASSET_IMPORTER_H

#include "core/engine/assetmanagement/assetmanager/assetimporter.h"

class MModelAssetImporter : public MAssetImporter
{
    DEFINE_OBJECT_SUBCLASS(MModelAssetImporter)
private:
    static bool registered;
public:
    bool canImport(SString fileExtension) override;
    MAsset* importAsset(SString path, const pugi::xml_document& metaData) override;
    [[nodiscard]] SString getIconPath() const override;
};

#endif // MODEL_ASSET_IMPORTER_H
