//
// package_asset_source.cpp
//

#include "core/package/package_asset_source.h"

#include "core/package/packaged_asset_manager.h"
#include "pugixml.hpp"

MPackageAssetSource::MPackageAssetSource(std::shared_ptr<MPackagedAssetManager> inPackage)
    : package(std::move(inPackage))
{
    // Every parent folder of every asset path: "a/b/c.png" -> "a", "a/b".
    package->manifest().forEachEntry([this](const SPackagedAssetEntry& e)
    {
        const SString& p = e.path;
        for (size_t slash = p.findFirst('/'); slash != SString::npos; slash = p.findFirst('/', slash + 1))
            directories.insert(p.substring(0, slash));
    });
}

SString MPackageAssetSource::getDebugName() const
{
    return SString("Package(") + SString(package->getDirectory().generic_string()) + ")";
}

bool MPackageAssetSource::exists(const SString& rawPath) const
{
    const SString path = MAssetPath::normalize(rawPath);
    if (package->manifest().findByPath(path))
        return true;
    if (MAssetPath::isMetaPath(path))
        return hasMeta(path.substring(0, path.size() - 5));
    return directories.contains(path);
}

void MPackageAssetSource::enumerate(const std::function<void(const SAssetSourceEntry&)>& fn) const
{
    for (const auto& dir : directories)
        fn(SAssetSourceEntry{ dir, true });

    package->manifest().forEachEntry([&](const SPackagedAssetEntry& e)
    {
        fn(SAssetSourceEntry{ e.path, false });
    });
}

bool MPackageAssetSource::readBytes(const SString& path, std::vector<uint8_t>& out) const
{
    return package->readAssetBytes(MAssetPath::normalize(path), out);
}

std::unique_ptr<IAssetStream> MPackageAssetSource::openStream(const SString& path) const
{
    return package->openStream(MAssetPath::normalize(path));
}

bool MPackageAssetSource::hasMeta(const SString& path) const
{
    const auto* entry = package->manifest().findByPath(MAssetPath::normalize(path));
    return entry && entry->metaSize > 0;
}

bool MPackageAssetSource::readMeta(const SString& path, pugi::xml_document& out) const
{
    return package->readAssetMeta(MAssetPath::normalize(path), out);
}
