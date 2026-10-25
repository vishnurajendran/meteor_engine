//
// packaged_asset_manager.cpp
//

#include "core/package/packaged_asset_manager.h"

#include <fstream>

#include "core/utils/logger.h"

// ---------------------------------------------------------------------------
// Stream over one asset's byte range inside a pak. Owns its own file handle,
// so separate streams can be used from separate threads.
// ---------------------------------------------------------------------------

namespace
{
class MPakRangeAssetStream final : public IAssetStream
{
public:
    bool open(const std::filesystem::path& pak, uint64_t inBase, uint64_t inLength)
    {
        file.open(pak, std::ios::binary | std::ios::in);
        if (!file.is_open()) return false;
        base   = inBase;
        length = inLength;
        file.seekg(static_cast<std::streamoff>(base), std::ios::beg);
        return static_cast<bool>(file);
    }

    size_t read(void* dst, size_t bytes) override
    {
        if (bytes == 0 || cursor >= length) return 0;
        const uint64_t n = std::min<uint64_t>(bytes, length - cursor);

        file.read(static_cast<char*>(dst), static_cast<std::streamsize>(n));
        const auto got = static_cast<size_t>(file.gcount());
        cursor += got;
        if (!file) file.clear();
        return got;
    }

    bool seek(uint64_t offset) override
    {
        if (offset > length) return false;
        file.clear();
        file.seekg(static_cast<std::streamoff>(base + offset), std::ios::beg);
        if (!file) { file.clear(); return false; }
        cursor = offset;
        return true;
    }

    uint64_t tell() const override { return cursor; }
    uint64_t size() const override { return length; }

private:
    std::ifstream file;
    uint64_t      base   = 0;
    uint64_t      length = 0;
    uint64_t      cursor = 0;
};

bool readU32(std::ifstream& in, uint32_t& out)
{
    uint8_t b[4];
    if (!in.read(reinterpret_cast<char*>(b), 4)) return false;
    out = b[0] | (b[1] << 8) | (b[2] << 16) | (static_cast<uint32_t>(b[3]) << 24);
    return true;
}
} // namespace

// ---------------------------------------------------------------------------
// Mounting
// ---------------------------------------------------------------------------

MPackagedAssetManager::~MPackagedAssetManager() = default;

std::filesystem::path MPackagedAssetManager::pakPath(uint32_t pakId) const
{
    return directory / pakFileName(pakId).str();
}

bool MPackagedAssetManager::validatePak(uint32_t pakId, SString& outError) const
{
    const auto path = pakPath(pakId);
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
    {
        outError = SString("missing ") + SString(path.filename().string());
        return false;
    }

    uint32_t magic = 0, version = 0, len = 0, index = 0;
    if (!readU32(in, magic) || magic != PAK_MAGIC)
    {
        outError = SString(path.filename().string()) + " is not a Meteor pak";
        return false;
    }
    if (!readU32(in, version) || version != FORMAT_VERSION)
    {
        outError = SString(path.filename().string()) + " has a different format version than the manifest";
        return false;
    }

    SString buildId;
    if (!readU32(in, len) || len > 1024)
    {
        outError = SString(path.filename().string()) + " has a corrupt header";
        return false;
    }
    buildId.resize(len);
    if (len > 0 && !in.read(&buildId[0], len))
    {
        outError = SString(path.filename().string()) + " has a corrupt header";
        return false;
    }
    if (buildId != assetManifest.getBuildId())
    {
        outError = SString(path.filename().string()) + " belongs to a different build than the manifest";
        return false;
    }
    if (!readU32(in, index) || index != pakId)
    {
        outError = SString(path.filename().string()) + " has the wrong pak index";
        return false;
    }
    return true;
}

bool MPackagedAssetManager::loadManifest(const std::filesystem::path& inDirectory)
{
    mounted   = false;
    directory = inDirectory;
    {
        std::lock_guard lock(loadedMutex);
        loadedByGuid.clear();
    }

    const auto manifestPath = directory / MANIFEST_FILE_NAME;
    std::error_code ec;
    if (!std::filesystem::exists(manifestPath, ec))
        return false;   // not a published game; caller falls back quietly

    SString error;
    if (!assetManifest.load(manifestPath, error))
    {
        MERROR(SString("PackagedAssetManager:: cannot mount package: ") + error);
        return false;
    }

    for (uint32_t i = 0; i < assetManifest.getPakCount(); ++i)
    {
        if (!validatePak(i, error))
        {
            MERROR(SString("PackagedAssetManager:: cannot mount package: ") + error);
            return false;
        }
    }

    mounted = true;
    MLOG(SString::format("PackagedAssetManager:: mounted {0} {1} ({2} assets, {3} paks, build {4})",
                         assetManifest.getGameInfo().name, assetManifest.getGameInfo().version,
                         SString::fromInt(static_cast<int>(assetManifest.getAssetCount())),
                         SString::fromInt(static_cast<int>(assetManifest.getPakCount())),
                         assetManifest.getBuildId()));
    return true;
}

// ---------------------------------------------------------------------------
// Reading
// ---------------------------------------------------------------------------

bool MPackagedAssetManager::readRange(uint32_t pakId, uint64_t offset, uint64_t size, uint8_t* dst) const
{
    if (size == 0) return true;

    std::ifstream in(pakPath(pakId), std::ios::binary);
    if (!in.is_open()) return false;
    in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(size));
    return static_cast<uint64_t>(in.gcount()) == size;
}

std::shared_ptr<const SPackagedAssetData> MPackagedAssetManager::findLoaded(const SString& guid) const
{
    std::lock_guard lock(loadedMutex);
    const auto it = loadedByGuid.find(guid);
    return it != loadedByGuid.end() ? it->second.lock() : nullptr;
}

std::shared_ptr<const SPackagedAssetData> MPackagedAssetManager::load(const SPackagedAssetEntry& entry)
{
    if (auto existing = findLoaded(entry.guid))
        return existing;

    auto data  = std::make_shared<SPackagedAssetData>();
    data->guid = entry.guid;
    data->blob.resize(static_cast<size_t>(entry.dataSize));

    std::vector<uint8_t> metaBytes(entry.metaSize);
    if (!readRange(entry.pakId, entry.offset, entry.metaSize, metaBytes.data()) ||
        !readRange(entry.pakId, entry.offset + entry.metaSize, entry.dataSize, data->blob.data()))
    {
        MERROR(SString("PackagedAssetManager:: failed to read ") + entry.path);
        return nullptr;
    }
    if (!metaBytes.empty())
        data->meta.load_buffer(metaBytes.data(), metaBytes.size());

    std::shared_ptr<const SPackagedAssetData> shared = data;
    std::lock_guard lock(loadedMutex);
    loadedByGuid[entry.guid] = shared;
    return shared;
}

std::shared_ptr<const SPackagedAssetData> MPackagedAssetManager::loadByGuid(const SString& guid)
{
    const auto* entry = mounted ? assetManifest.findByGuid(guid) : nullptr;
    return entry ? load(*entry) : nullptr;
}

std::shared_ptr<const SPackagedAssetData> MPackagedAssetManager::loadByPath(const SString& path)
{
    const auto* entry = mounted ? assetManifest.findByPath(path) : nullptr;
    return entry ? load(*entry) : nullptr;
}

void MPackagedAssetManager::unloadByGuids(const std::vector<SString>& guids)
{
    std::lock_guard lock(loadedMutex);
    for (const auto& guid : guids)
        loadedByGuid.erase(guid);
}

bool MPackagedAssetManager::loadCoreAsset(const SString& name, std::vector<uint8_t>& out) const
{
    const auto* entry = mounted ? assetManifest.findCoreAsset(name) : nullptr;
    if (!entry) return false;

    out.resize(static_cast<size_t>(entry->size));
    return readRange(entry->pakId, entry->offset, entry->size, out.data());
}

bool MPackagedAssetManager::readAssetBytes(const SString& path, std::vector<uint8_t>& out) const
{
    const auto* entry = mounted ? assetManifest.findByPath(path) : nullptr;
    if (!entry) return false;

    if (const auto loaded = findLoaded(entry->guid))
    {
        out = loaded->blob;
        return true;
    }

    out.resize(static_cast<size_t>(entry->dataSize));
    return readRange(entry->pakId, entry->offset + entry->metaSize, entry->dataSize, out.data());
}

bool MPackagedAssetManager::readAssetMeta(const SString& path, pugi::xml_document& out) const
{
    const auto* entry = mounted ? assetManifest.findByPath(path) : nullptr;
    if (!entry || entry->metaSize == 0) return false;

    std::vector<uint8_t> bytes(entry->metaSize);
    if (!readRange(entry->pakId, entry->offset, entry->metaSize, bytes.data()))
        return false;
    return out.load_buffer(bytes.data(), bytes.size()).status == pugi::status_ok;
}

std::unique_ptr<IAssetStream> MPackagedAssetManager::openStream(const SString& path) const
{
    const auto* entry = mounted ? assetManifest.findByPath(path) : nullptr;
    if (!entry) return nullptr;

    auto stream = std::make_unique<MPakRangeAssetStream>();
    if (!stream->open(pakPath(entry->pakId), entry->offset + entry->metaSize, entry->dataSize))
        return nullptr;
    return stream;
}
