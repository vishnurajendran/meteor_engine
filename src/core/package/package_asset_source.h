//
// package_asset_source.h
//
// Read-only IAssetSource over a mounted package. This is how the rest of the
// engine (importers, Assimp IO, the miniaudio VFS, scene loading) reads a
// published game without knowing it is packaged.
//
// Folders are not stored in the package; they are worked out from the asset
// paths so enumerate() and exists() behave like the directory source.
//

#ifndef PACKAGE_ASSET_SOURCE_H
#define PACKAGE_ASSET_SOURCE_H

#include <memory>
#include <set>

#include "core/engine/assetmanagement/source/asset_source.h"

class MPackagedAssetManager;

class MPackageAssetSource final : public IAssetSource
{
public:
    explicit MPackageAssetSource(std::shared_ptr<MPackagedAssetManager> package);

    [[nodiscard]] MPackagedAssetManager& getPackage() const { return *package; }

    // -- IAssetSource -----------------------------------------------------------
    SString getDebugName() const override;

    bool exists(const SString& path) const override;
    void enumerate(const std::function<void(const SAssetSourceEntry&)>& fn) const override;

    bool readBytes(const SString& path, std::vector<uint8_t>& out) const override;
    std::unique_ptr<IAssetStream> openStream(const SString& path) const override;

    bool hasMeta(const SString& path) const override;
    bool readMeta(const SString& path, pugi::xml_document& out) const override;

private:
    std::shared_ptr<MPackagedAssetManager> package;
    std::set<SString>                      directories;
};

#endif // PACKAGE_ASSET_SOURCE_H
