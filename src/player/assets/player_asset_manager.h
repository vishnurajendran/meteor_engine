//
// player_asset_manager.h
//
// Asset manager for the player.
//
// Packaged (a .manifest next to the executable):
//   refresh() only mounts the package; nothing is imported. The first lookup
//   of an asset (by path or GUID) imports it from the package, runs its
//   deferred load straight away, and keeps it for the rest of the session.
//   Dependencies (a cubemap's faces, a material's shader) load the same way
//   through their own lookups.
//
// Not packaged (running the player from a project folder during
// development): behaves exactly like MAssetManager and imports everything.
//
// Lookups happen on the main thread, like every other asset manager call.
//

#ifndef PLAYER_ASSET_MANAGER_H
#define PLAYER_ASSET_MANAGER_H

#include <memory>
#include <set>

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"

class MPackagedAssetManager;

class MPlayerAssetManager : public MAssetManager
{
    DEFINE_OBJECT_SUBCLASS(MPlayerAssetManager)

public:
    // Switches the manager to the package and makes it the active asset
    // source. Call before refresh().
    void setPackage(std::shared_ptr<MPackagedAssetManager> package);

    [[nodiscard]] bool isPackaged() const { return package != nullptr; }
    [[nodiscard]] std::shared_ptr<MPackagedAssetManager> getPackage() const { return package; }

    void refresh() override;
    [[nodiscard]] std::vector<SString> listAssetPaths() const override;

    // Core assets come from the package (none when not packaged).
    [[nodiscard]] bool providesCoreAssets() const override { return package != nullptr; }
    [[nodiscard]] std::vector<SString> listCoreAssets(const SString& prefix) const override;
    bool loadCoreAsset(const SString& name, SString& outData) const override;

protected:
    MAsset* loadOnDemand(const SString& path) override;
    MAsset* loadOnDemandById(const SString& assetId) override;

private:
    std::shared_ptr<MPackagedAssetManager> package;
    std::set<SString> loadingPaths;   // guards against an asset requesting itself while importing
    std::set<SString> failedPaths;    // import failed once; don't retry (and re-log) on every lookup
    std::set<SString> missingReported; // paths / GUIDs already reported as not in the package
};

#endif // PLAYER_ASSET_MANAGER_H
