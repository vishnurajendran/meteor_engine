//
// player_asset_manager.cpp
//

#include "player/assets/player_asset_manager.h"

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/source/asset_source.h"
#include "core/package/package_asset_source.h"
#include "core/package/packaged_asset_manager.h"
#include "core/utils/logger.h"

void MPlayerAssetManager::setPackage(std::shared_ptr<MPackagedAssetManager> inPackage)
{
    package = std::move(inPackage);
    if (package)
        setAssetSource(std::make_shared<MPackageAssetSource>(package));
}

void MPlayerAssetManager::refresh()
{
    if (!package)
    {
        MAssetManager::refresh();
        return;
    }

    cleanup();
    loadingPaths.clear();
    failedPaths.clear();
    missingReported.clear();
    MLOG(SString::format("PlayerAssetManager:: package mounted, {0} assets available (loaded on first use)",
                         SString::fromInt(static_cast<int>(package->manifest().getAssetCount()))));
}

std::vector<SString> MPlayerAssetManager::listAssetPaths() const
{
    if (!package)
        return MAssetManager::listAssetPaths();

    std::vector<SString> paths;
    paths.reserve(package->manifest().getAssetCount());
    package->manifest().forEachEntry([&](const SPackagedAssetEntry& e) { paths.push_back(e.path); });
    return paths;
}

std::vector<SString> MPlayerAssetManager::listCoreAssets(const SString& prefix) const
{
    std::vector<SString> names;
    if (package)
        package->manifest().forEachCoreAsset(prefix, [&](const SPackagedCoreAssetEntry& e) { names.push_back(e.name); });
    return names;
}

bool MPlayerAssetManager::loadCoreAsset(const SString& name, SString& outData) const
{
    std::vector<uint8_t> bytes;
    if (!package || !package->loadCoreAsset(name, bytes))
        return false;
    outData = SString(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}

MAsset* MPlayerAssetManager::loadOnDemand(const SString& rawPath)
{
    if (!package || rawPath.empty())
        return nullptr;

    const SString path = MAssetPath::normalize(rawPath);
    if (const auto it = assetMap.find(path); it != assetMap.end())
        return it->second;

    if (failedPaths.contains(path) || loadingPaths.contains(path))
        return nullptr;
    if (!package->manifest().findByPath(path))
    {
        // Reported once per path: the lookup itself failed, the asset was
        // never published (as opposed to "failed to import" below).
        if (missingReported.insert(path).second)
            MWARN(SString("PlayerAssetManager:: not in the package: ") + path);
        return nullptr;
    }

    loadingPaths.insert(path);
    const bool loaded = loadAsset(path);
    loadingPaths.erase(path);

    if (!loaded)
    {
        failedPaths.insert(path);
        MERROR(SString("PlayerAssetManager:: failed to import ") + path +
               " - it is in the package, but its importer rejected it (see the errors just above). "
               "Later lookups of it will fail without retrying.");
        return nullptr;
    }

    MAsset* asset = assetMap[path];
    // The editor runs deferred loads after importing everything. Here each
    // asset finishes as soon as it is imported; anything it depends on is
    // pulled in through its own lookups.
    if (asset && asset->hasDeferredLoad())
        asset->deferredAssetLoad(false);

    MVERBOSE(SString("PlayerAssetManager:: loaded ") + path);
    return asset;
}

MAsset* MPlayerAssetManager::loadOnDemandById(const SString& assetId)
{
    if (!package || assetId.empty())
        return nullptr;

    const auto* entry = package->manifest().findByGuid(assetId);
    if (!entry)
    {
        if (missingReported.insert(SString("guid:") + assetId).second)
            MWARN(SString("PlayerAssetManager:: no asset with GUID ") + assetId + " in the package");
        return nullptr;
    }

    MAsset* asset = loadOnDemand(entry->path);
    if (const auto it = assetMapByAssetId.find(assetId); it != assetMapByAssetId.end())
        return it->second;
    return asset;
}
