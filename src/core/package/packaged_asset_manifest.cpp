//
// packaged_asset_manifest.cpp
//

#include "core/package/packaged_asset_manifest.h"

#include <fstream>

bool readWholeFile(const std::filesystem::path& file, std::vector<uint8_t>& out)
{
    std::ifstream in(file, std::ios::binary | std::ios::ate);
    if (!in.is_open())
        return false;

    const auto end = in.tellg();
    if (end < 0)
        return false;

    out.resize(static_cast<size_t>(end));
    in.seekg(0, std::ios::beg);
    if (!out.empty())
        in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));
    return static_cast<bool>(in) || in.eof();
}

// ---------------------------------------------------------------------------
// Reading
// ---------------------------------------------------------------------------

void SPackagedAssetManifest::clear()
{
    formatVersion = FORMAT_VERSION;
    buildId       = SString();
    pakCount      = 0;
    gameInfo      = {};
    startupScene  = {};
    settings      = {};
    guidToAsset.clear();
    pathToGuid.clear();
    coreAssets.clear();
}

bool SPackagedAssetManifest::load(const std::filesystem::path& file, SString& outError)
{
    clear();

    std::vector<uint8_t> bytes;
    if (!readWholeFile(file, bytes))
    {
        outError = SString("cannot read ") + SString(file.generic_string());
        return false;
    }

    SByteReader r(bytes.data(), bytes.size());

    uint32_t magic = 0, assetCount = 0;
    if (!r.u32(magic) || magic != MANIFEST_MAGIC)
    {
        outError = "not a Meteor manifest (bad magic)";
        return false;
    }
    if (!r.u32(formatVersion))
    {
        outError = "manifest header is truncated";
        return false;
    }
    if (formatVersion != FORMAT_VERSION)
    {
        outError = SString::format("manifest format {0} is not supported by this player (expects {1})",
                                   SString::fromInt(static_cast<int>(formatVersion)),
                                   SString::fromInt(static_cast<int>(FORMAT_VERSION)));
        return false;
    }
    if (!r.str(buildId) || !r.u32(pakCount) || !r.u32(assetCount))
    {
        outError = "manifest header is truncated";
        return false;
    }

    // ASST rows in file order, so PATH can refer to them by index.
    std::vector<SString> assetGuidsInOrder;
    bool hasPathSection = false;

    while (!r.atEnd())
    {
        uint32_t tag = 0, size = 0;
        if (!r.u32(tag) || !r.u32(size) || size > r.remaining())
        {
            outError = "manifest section header is truncated";
            return false;
        }

        SByteReader s(r.cursor(), size);
        r.skip(size);

        switch (tag)
        {
            case SECTION_INFO:
                s.str(gameInfo.name);
                s.str(gameInfo.id);
                s.str(gameInfo.version);
                s.str(gameInfo.publisher);
                s.str(gameInfo.engineVersion);
                break;

            case SECTION_START:
                s.str(startupScene.guid);
                s.str(startupScene.path);
                break;

            case SECTION_SETTINGS:
                s.str(settings.physicsLayersXml);
                s.str(settings.engineSettingsXml);
                break;

            case SECTION_CORE:
            {
                uint32_t count = 0;
                s.u32(count);
                for (uint32_t i = 0; i < count && s.ok(); ++i)
                {
                    SPackagedCoreAssetEntry e;
                    s.str(e.name);
                    s.u32(e.pakId);
                    s.u64(e.offset);
                    s.u64(e.size);
                    if (!s.ok()) break;

                    if (e.pakId >= pakCount)
                    {
                        outError = SString("core asset refers to a missing pak: ") + e.name;
                        return false;
                    }
                    coreAssets[e.name] = e;
                }
                break;
            }

            case SECTION_ASSETS:
            {
                uint32_t count = 0;
                s.u32(count);
                assetGuidsInOrder.reserve(count);
                for (uint32_t i = 0; i < count && s.ok(); ++i)
                {
                    SPackagedAssetEntry e;
                    s.str(e.guid);
                    s.str(e.path);
                    s.u32(e.pakId);
                    s.u64(e.offset);
                    s.u32(e.metaSize);
                    s.u64(e.dataSize);
                    if (!s.ok()) break;

                    if (e.pakId >= pakCount)
                    {
                        outError = SString::format("asset {0} refers to pak {1}, but the build has {2}",
                                                   e.path, SString::fromInt(static_cast<int>(e.pakId)),
                                                   SString::fromInt(static_cast<int>(pakCount)));
                        return false;
                    }
                    assetGuidsInOrder.push_back(e.guid);
                    guidToAsset[e.guid] = e;
                }
                break;
            }

            case SECTION_PATHS:
            {
                hasPathSection = true;
                uint32_t count = 0;
                s.u32(count);
                for (uint32_t i = 0; i < count && s.ok(); ++i)
                {
                    SString  path;
                    uint32_t index = 0;
                    s.str(path);
                    s.u32(index);
                    if (!s.ok()) break;

                    if (index >= assetGuidsInOrder.size())
                    {
                        outError = SString("path table refers to a missing asset: ") + path;
                        return false;
                    }
                    pathToGuid[path] = assetGuidsInOrder[index];
                }
                break;
            }

            default:
                // Section from a newer engine; skip it.
                break;
        }

        if (!s.ok())
        {
            outError = SString::format("manifest section {0} is corrupt",
                                       SString::fromInt(static_cast<int>(tag)));
            return false;
        }
    }

    if (guidToAsset.size() != assetCount || !hasPathSection || pathToGuid.size() != assetCount)
    {
        outError = "manifest asset tables are incomplete";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const SPackagedAssetEntry* SPackagedAssetManifest::findByGuid(const SString& guid) const
{
    const auto it = guidToAsset.find(guid);
    return it != guidToAsset.end() ? &it->second : nullptr;
}

bool SPackagedAssetManifest::findGuidByPath(const SString& path, SString& outGuid) const
{
    const auto it = pathToGuid.find(path);
    if (it == pathToGuid.end())
        return false;
    outGuid = it->second;
    return true;
}

const SPackagedAssetEntry* SPackagedAssetManifest::findByPath(const SString& path) const
{
    SString guid;
    return findGuidByPath(path, guid) ? findByGuid(guid) : nullptr;
}

const SPackagedCoreAssetEntry* SPackagedAssetManifest::findCoreAsset(const SString& name) const
{
    const auto it = coreAssets.find(name);
    return it != coreAssets.end() ? &it->second : nullptr;
}

void SPackagedAssetManifest::forEachCoreAsset(const SString& prefix,
                                              const std::function<void(const SPackagedCoreAssetEntry&)>& fn) const
{
    for (const auto& [name, e] : coreAssets)
        if (prefix.empty() || name.startsWith(prefix))
            fn(e);
}

void SPackagedAssetManifest::forEachEntry(const std::function<void(const SPackagedAssetEntry&)>& fn) const
{
    for (const auto& [path, guid] : pathToGuid)
        if (const auto* e = findByGuid(guid))
            fn(*e);
}

// ---------------------------------------------------------------------------
// Writing
// ---------------------------------------------------------------------------

bool SPackagedAssetManifest::addCoreAsset(const SPackagedCoreAssetEntry& entry)
{
    if (coreAssets.contains(entry.name))
        return false;
    coreAssets[entry.name] = entry;
    return true;
}

bool SPackagedAssetManifest::addEntry(const SPackagedAssetEntry& entry)
{
    if (guidToAsset.contains(entry.guid) || pathToGuid.contains(entry.path))
        return false;
    guidToAsset[entry.guid] = entry;
    pathToGuid[entry.path]  = entry.guid;
    return true;
}

std::vector<uint8_t> SPackagedAssetManifest::serialise() const
{
    SByteWriter w;
    w.u32(MANIFEST_MAGIC);
    w.u32(FORMAT_VERSION);
    w.str(buildId);
    w.u32(pakCount);
    w.u32(static_cast<uint32_t>(guidToAsset.size()));

    {
        SByteWriter s;
        s.str(gameInfo.name);
        s.str(gameInfo.id);
        s.str(gameInfo.version);
        s.str(gameInfo.publisher);
        s.str(gameInfo.engineVersion);
        w.section(SECTION_INFO, s);
    }
    {
        SByteWriter s;
        s.str(startupScene.guid);
        s.str(startupScene.path);
        w.section(SECTION_START, s);
    }
    {
        SByteWriter s;
        s.str(settings.physicsLayersXml);
        s.str(settings.engineSettingsXml);
        w.section(SECTION_SETTINGS, s);
    }
    {
        SByteWriter s;
        s.u32(static_cast<uint32_t>(coreAssets.size()));
        for (const auto& [name, e] : coreAssets)
        {
            s.str(e.name);
            s.u32(e.pakId);
            s.u64(e.offset);
            s.u64(e.size);
        }
        w.section(SECTION_CORE, s);
    }

    // ASST is written in GUID order (std::map), PATH in path order; both are
    // deterministic, so the same input always produces the same file.
    std::map<SString, uint32_t> indexByGuid;
    {
        SByteWriter s;
        s.u32(static_cast<uint32_t>(guidToAsset.size()));
        uint32_t index = 0;
        for (const auto& [guid, e] : guidToAsset)
        {
            s.str(e.guid);
            s.str(e.path);
            s.u32(e.pakId);
            s.u64(e.offset);
            s.u32(e.metaSize);
            s.u64(e.dataSize);
            indexByGuid[guid] = index++;
        }
        w.section(SECTION_ASSETS, s);
    }
    {
        SByteWriter s;
        s.u32(static_cast<uint32_t>(pathToGuid.size()));
        for (const auto& [path, guid] : pathToGuid)
        {
            s.str(path);
            s.u32(indexByGuid[guid]);
        }
        w.section(SECTION_PATHS, s);
    }

    return w.bytes;
}
