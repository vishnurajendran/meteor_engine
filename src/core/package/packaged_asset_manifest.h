//
// packaged_asset_manifest.h
//
// The .manifest file of a published game (layout in package_format.h).
//
// Holds the game info, startup scene, embedded settings and the two asset
// tables:
//
//   GUID -> asset   where an asset lives (pak, offset, sizes) and its path
//   path -> GUID    how asset paths are resolved
//
// It also lists core assets: engine files stored by name only (no meta, no
// GUID), such as the shader include files.
//
// load() is used by the player; serialise() by the editor's publisher, so
// the reading and writing of this file sit side by side.
//
// Read-only after load(); safe to query from any thread.
//

#ifndef PACKAGED_ASSET_MANIFEST_H
#define PACKAGED_ASSET_MANIFEST_H

#include <filesystem>
#include <functional>
#include <map>
#include <vector>

#include "core/package/package_format.h"

// One row of the GUID -> asset table. Holds no memory; safe to copy.
struct SPackagedAssetEntry
{
    SString  guid;
    SString  path;          // the importer is chosen by extension
    uint32_t pakId    = 0;
    uint64_t offset   = 0;  // absolute offset of the meta bytes in the pak
    uint32_t metaSize = 0;
    uint64_t dataSize = 0;  // data starts at offset + metaSize
};

// An engine file stored by name. Holds no memory; safe to copy.
struct SPackagedCoreAssetEntry
{
    SString  name;          // e.g. "shader_utils/meteor_vertex_utils.glsl"
    uint32_t pakId  = 0;
    uint64_t offset = 0;    // absolute offset of the data in the pak
    uint64_t size   = 0;
};

class SPackagedAssetManifest
{
public:
    // Reads and validates a .manifest file. On failure returns false and
    // describes the problem in outError.
    bool load(const std::filesystem::path& file, SString& outError);

    // -- Asset tables -----------------------------------------------------------
    [[nodiscard]] const SPackagedAssetEntry* findByGuid(const SString& guid) const;
    [[nodiscard]] const SPackagedAssetEntry* findByPath(const SString& path) const;
    bool findGuidByPath(const SString& path, SString& outGuid) const;

    // Sorted by path.
    void forEachEntry(const std::function<void(const SPackagedAssetEntry&)>& fn) const;
    [[nodiscard]] size_t getAssetCount() const { return guidToAsset.size(); }

    // -- Core assets ------------------------------------------------------------
    [[nodiscard]] const SPackagedCoreAssetEntry* findCoreAsset(const SString& name) const;
    // Sorted by name. Only names starting with `prefix` (all if empty).
    void forEachCoreAsset(const SString& prefix,
                          const std::function<void(const SPackagedCoreAssetEntry&)>& fn) const;

    // -- Header / sections ----------------------------------------------------
    [[nodiscard]] uint32_t getFormatVersion() const { return formatVersion; }
    [[nodiscard]] const SString& getBuildId() const { return buildId; }
    [[nodiscard]] uint32_t getPakCount()      const { return pakCount; }

    [[nodiscard]] const SGameInfo&         getGameInfo()     const { return gameInfo; }
    [[nodiscard]] const SStartupScene&     getStartupScene() const { return startupScene; }
    [[nodiscard]] const SEmbeddedSettings& getSettings()     const { return settings; }

    // -- Writing (editor publisher) -------------------------------------------
    void setBuildId(const SString& id)               { buildId = id; }
    void setPakCount(uint32_t count)                 { pakCount = count; }
    void setGameInfo(const SGameInfo& info)          { gameInfo = info; }
    void setStartupScene(const SStartupScene& scene) { startupScene = scene; }
    void setSettings(const SEmbeddedSettings& s)     { settings = s; }

    // Returns false if the GUID or path is already present.
    bool addEntry(const SPackagedAssetEntry& entry);
    // Returns false if the name is already present.
    bool addCoreAsset(const SPackagedCoreAssetEntry& entry);

    // Builds the complete .manifest file contents.
    [[nodiscard]] std::vector<uint8_t> serialise() const;

private:
    void clear();

    uint32_t formatVersion = FORMAT_VERSION;
    SString  buildId;
    uint32_t pakCount = 0;

    SGameInfo         gameInfo;
    SStartupScene     startupScene;
    SEmbeddedSettings settings;

    std::map<SString, SPackagedAssetEntry> guidToAsset;  // table A
    std::map<SString, SString>             pathToGuid;   // table B
    std::map<SString, SPackagedCoreAssetEntry> coreAssets;  // by name
};

#endif // PACKAGED_ASSET_MANIFEST_H
