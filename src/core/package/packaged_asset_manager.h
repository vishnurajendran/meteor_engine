//
// packaged_asset_manager.h
//
// Reads assets out of a published game (.manifest + NN.pak).
//
//   loadManifest()   reads the .manifest and checks every pak's header
//                    (magic, format version, build ID, index). The player
//                    refuses to mount a package that fails any check.
//
//   loadByGuid()     reads one asset's data and meta into memory. Requests
//   loadByPath()     for an asset that is still in use share one copy;
//                    memory is freed when the last user releases it.
//
//   openStream()     seekable stream over one asset's bytes, read directly
//                    from the pak (used for audio streaming).
//
//   loadCoreAsset()  an engine file stored by name, without meta (e.g. the
//                    shader include files).
//
// The engine never talks to this directly. Assets go through
// MPackageAssetSource (an IAssetSource), so importers do not know whether
// bytes come from a folder or a package; core assets go through the asset
// manager (MPlayerAssetManager::loadCoreAsset).
//
// Thread-safe: every read opens its own file handle, and the loaded-asset
// cache is guarded by a mutex. Audio is streamed on miniaudio's threads.
//

#ifndef PACKAGED_ASSET_MANAGER_H
#define PACKAGED_ASSET_MANAGER_H

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "pugixml.hpp"
#include "core/object/object.h"
#include "core/package/packaged_asset_manifest.h"
#include "core/engine/assetmanagement/source/asset_stream.h"

// Bytes for one asset, owned. Freed automatically when the last reference
// goes away.
struct SPackagedAssetData
{
    SString              guid;
    std::vector<uint8_t> blob;   // raw asset data; blob.size() is its size
    pugi::xml_document   meta;   // engine meta (GUID, import settings)
};

class MPackagedAssetManager : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MPackagedAssetManager)

public:
    MPackagedAssetManager() = default;
    ~MPackagedAssetManager() override;

    // `directory` holds the .manifest and the paks. Returns false (and logs
    // why) if the manifest is missing or any pak does not belong to it.
    bool loadManifest(const std::filesystem::path& directory);

    [[nodiscard]] bool isMounted() const { return mounted; }
    [[nodiscard]] const std::filesystem::path& getDirectory() const { return directory; }
    [[nodiscard]] const SPackagedAssetManifest& manifest() const { return assetManifest; }

    // -- Loading --------------------------------------------------------------
    // nullptr if the asset is not in the package or cannot be read.
    std::shared_ptr<const SPackagedAssetData> loadByGuid(const SString& guid);
    std::shared_ptr<const SPackagedAssetData> loadByPath(const SString& path);

    // Drops the cache's references. Memory is freed once callers release
    // theirs, so this is safe to call while an asset is still in use.
    void unloadByGuids(const std::vector<SString>& guids);

    // -- Core assets ------------------------------------------------------------
    // Reads an engine file stored by name. False if it is not in the package.
    bool loadCoreAsset(const SString& name, std::vector<uint8_t>& out) const;

    // -- Used by MPackageAssetSource ------------------------------------------
    // Copy of an asset's data. Reuses a loaded copy when there is one.
    bool readAssetBytes(const SString& path, std::vector<uint8_t>& out) const;
    bool readAssetMeta(const SString& path, pugi::xml_document& out) const;
    [[nodiscard]] std::unique_ptr<IAssetStream> openStream(const SString& path) const;

private:
    [[nodiscard]] std::filesystem::path pakPath(uint32_t pakId) const;
    bool readRange(uint32_t pakId, uint64_t offset, uint64_t size, uint8_t* dst) const;
    bool validatePak(uint32_t pakId, SString& outError) const;
    std::shared_ptr<const SPackagedAssetData> load(const SPackagedAssetEntry& entry);
    std::shared_ptr<const SPackagedAssetData> findLoaded(const SString& guid) const;

    SPackagedAssetManifest assetManifest;
    std::filesystem::path  directory;
    bool                   mounted = false;

    mutable std::mutex loadedMutex;
    std::map<SString, std::weak_ptr<const SPackagedAssetData>> loadedByGuid;
};

#endif // PACKAGED_ASSET_MANAGER_H
