//
// directory_asset_source.h
//
// Asset source backed by loose files on disk. Used by the editor (and by
// development player builds until packages exist).
//
//   root         Directory that asset paths are relative to. Empty means the
//                process working directory, which is how the engine resolved
//                paths before this class existed.
//   searchPaths  Sub-directories of root that enumerate() walks, e.g.
//                { "assets", "engine_assets" }. Reads are NOT restricted to
//                these; any path under root can be read.
//
// Thread-safe: holds no mutable state after construction. Each stream opens
// its own file handle.
//

#ifndef DIRECTORY_ASSET_SOURCE_H
#define DIRECTORY_ASSET_SOURCE_H

#include <filesystem>
#include <vector>

#include "core/engine/assetmanagement/source/asset_source.h"

class MDirectoryAssetSource final : public IWritableAssetSource
{
public:
    MDirectoryAssetSource(const SString& root, std::vector<SString> searchPaths);

    const std::filesystem::path& getRoot()        const { return rootPath; }
    const std::vector<SString>&  getSearchPaths() const { return searchPaths; }

    // -- IAssetSource -----------------------------------------------------------
    SString getDebugName() const override;

    bool exists(const SString& path) const override;
    void enumerate(const std::function<void(const SAssetSourceEntry&)>& fn) const override;

    bool readBytes(const SString& path, std::vector<uint8_t>& out) const override;
    std::unique_ptr<IAssetStream> openStream(const SString& path) const override;

    bool hasMeta(const SString& path) const override;
    bool readMeta(const SString& path, pugi::xml_document& out) const override;

    SString getDiskPath(const SString& path) const override;

    // -- IWritableAssetSource ---------------------------------------------------
    bool writeBytes(const SString& path, const void* data, size_t size) override;
    bool writeMeta(const SString& path, const pugi::xml_document& doc) override;
    bool remove(const SString& path) override;
    bool createDirectory(const SString& path) override;
    bool removeDirectory(const SString& path) override;

    // -- Path mapping (thread-safe; used by the editor's file watcher) ----------
    // Asset path -> path on disk (relative to the working dir if root is empty).
    std::filesystem::path resolve(const SString& path) const;
    // Path on disk -> normalized asset path.
    SString               toAssetPath(const std::filesystem::path& p) const;

private:
    std::filesystem::path rootPath;      // empty = working directory
    std::vector<SString>  searchPaths;
};

#endif // DIRECTORY_ASSET_SOURCE_H
