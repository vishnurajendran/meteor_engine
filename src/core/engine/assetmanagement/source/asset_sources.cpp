//
// asset_sources.cpp
//

#include "core/engine/assetmanagement/source/asset_sources.h"

#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/assetmanagement/source/directory_asset_source.h"
#include "core/engine/subsystem/subsystem_registry.h"

std::shared_ptr<IAssetSource> MAssetSources::getActive()
{
    if (auto* manager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>())
        return manager->getActiveAssetSource();

    // No asset manager (yet): plain files relative to the working directory.
    static const auto fallback = std::make_shared<MDirectoryAssetSource>(SString(""), std::vector<SString>{});
    return fallback;
}

std::shared_ptr<IWritableAssetSource> MAssetSources::getWritable()
{
    auto active = getActive();
    if (auto* writable = active->asWritable())
        return std::shared_ptr<IWritableAssetSource>(active, writable);   // aliasing ctor
    return nullptr;
}
