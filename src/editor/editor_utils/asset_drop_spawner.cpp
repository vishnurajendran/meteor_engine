//
// asset_drop_spawner.cpp
//

#include "asset_drop_spawner.h"

#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/audio/asset/audioclip_asset.h"
#include "core/engine/audio/audio_entity/audio_source_entity.h"
#include "core/engine/composition/composition_asset.h"
#include "core/engine/composition/composition_utility.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scene/scenemanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "editor/app/editorapplication.h"

namespace
{
IAssetManagerSubsystem* assetManager()
{
    return MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
}

bool hasActiveScene()
{
    auto* sm = MSceneManager::getSceneManagerInstance();
    return sm && sm->getActiveScene();
}

// "assets/mesh/monkey.glb" → "monkey" — reads better in the hierarchy than
// the asset's internal name ("Asset_monkey.glb").
SString entityNameFor(const MAsset* asset)
{
    return FileIO::getFileNameWithoutExtension(asset->getPath());
}
} // namespace

bool MAssetDropSpawner::canSpawn(const SString& assetId)
{
    auto* am = assetManager();
    if (!am || assetId.empty())
        return false;

    // Each check is one map lookup + dynamic_cast.
    return am->getAssetById<MStaticMeshAsset>(assetId).isValid()
        || am->getAssetById<MAudioClipAsset>(assetId).isValid()
        || am->getAssetById<MCompositionAsset>(assetId).isValid();
}

MSpatialEntity* MAssetDropSpawner::spawn(const SString& assetId)
{
    auto* am = assetManager();
    if (!am || !hasActiveScene())
        return nullptr;

    MSpatialEntity* entity = nullptr;

    if (const auto mesh = am->getAssetById<MStaticMeshAsset>(assetId))
    {
        auto* meshEntity = MSpatialEntity::createInstance<MStaticMeshEntity>(entityNameFor(mesh.get()));
        meshEntity->setStaticMeshAsset(mesh);
        entity = meshEntity;
    }
    else if (const auto clip = am->getAssetById<MAudioClipAsset>(assetId))
    {
        // createInstance → registerEntity → onCreate initialises the source,
        // so setClip works straight away.
        auto* source = MSpatialEntity::createInstance<MAudioSource>(entityNameFor(clip.get()));
        source->setClip(clip);
        entity = source;
    }
    else if (const auto comp = am->getAssetById<MCompositionAsset>(assetId))
    {
        // Keeps the root name stored in the .comp, and links the instance.
        entity = MCompositionUtility::instantiate(comp.get());
    }

    if (entity)
    {
        MEditorApplication::SelectedObject = entity;
        MLOG(SString::format("AssetDrop:: created '{0}'", entity->getName()));
    }
    return entity;
}
