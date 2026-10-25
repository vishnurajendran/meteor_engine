//
// game_publisher.cpp
//

#include "editor/publishing/game_publisher.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/assetmanagement/source/asset_source.h"
#include "core/engine/engine_statics.h"
#include "core/engine/settings/engine_settings.h"
#include "core/engine/settings/player_settings.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/package/packaged_asset_manifest.h"
#include "core/utils/guid.h"
#include "core/utils/logger.h"
#include "core/utils/meteor_paths.h"
#include "editor/app/editorapplication.h"
#include "pugixml.hpp"

namespace fs = std::filesystem;

namespace
{
#if defined(_WIN32)
    constexpr const char* EXE_SUFFIX = ".exe";
    constexpr const char* LIB_SUFFIX = ".dll";
#elif defined(__APPLE__)
    constexpr const char* EXE_SUFFIX = "";
    constexpr const char* LIB_SUFFIX = ".dylib";
#else
    constexpr const char* EXE_SUFFIX = "";
    constexpr const char* LIB_SUFFIX = ".so";
#endif

    constexpr const char* PLAYER_EXECUTABLE   = "MeteorPlayer";
    constexpr const char* PLAYER_SETTINGS     = "PlayerSettings.xml";
    constexpr const char* STAGING_FOLDER      = ".publish_staging";

    // Engine files the player looks up by name, packed as core assets:
    // { engine folder, file extension, core asset prefix }.
    struct SCoreAssetSource { const char* folder; const char* extension; const char* prefix; };
    constexpr SCoreAssetSource CORE_ASSET_SOURCES[] = {
        { "meteor_assets/shader_utils",         ".glsl", IAssetManagerSubsystem::CORE_SHADER_UTILS },   // shader #include files
        { ".engine_data/scripting/modules/lua", ".lua",  IAssetManagerSubsystem::CORE_LUA_MODULES  },   // Behaviour, MMath, ...
    };

    // Shader include files are core assets, not regular assets.
    constexpr const char* SHADER_UTILS_FOLDER = "meteor_assets/shader_utils";

    // Meta document tags; must match IAssetManagerSubsystem.
    constexpr const char* META_TAG     = "asset_id";
    constexpr const char* META_ID_ATTR = "id";

    bool isInside(const fs::path& child, const fs::path& parent)
    {
        const fs::path rel = child.lexically_normal().lexically_relative(parent.lexically_normal());
        return !rel.empty() && *rel.begin() != "..";
    }

    bool isSameOrInside(const fs::path& child, const fs::path& parent)
    {
        return child.lexically_normal() == parent.lexically_normal() || isInside(child, parent);
    }

    // File-name-safe version of the game name, for the executable.
    SString executableName(const SString& gameName)
    {
        SString out;
        for (const char c : gameName)
        {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '_')
                out += c;
        }
        out.trim();
        return out.empty() ? SString("Game") : out;
    }

    SString serialiseMeta(const pugi::xml_document& doc)
    {
        std::ostringstream oss;
        doc.save(oss, "", pugi::format_raw);
        return SString(oss.str());
    }

    SString formatBytes(uint64_t bytes)
    {
        const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        std::ostringstream oss;
        oss.precision(1);
        oss << std::fixed << mb << " MB";
        return SString(oss.str());
    }

    bool writeAll(std::ofstream& out, const void* data, size_t size)
    {
        if (size > 0)
            out.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
        return static_cast<bool>(out);
    }
} // namespace

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

MGamePublisher& MGamePublisher::get()
{
    static MGamePublisher instance;
    return instance;
}

bool MGamePublisher::isPublishing()
{
    return get().getState() == EPublishState::Running;
}

MGamePublisher::~MGamePublisher()
{
    cancelFlag = true;
    if (thread.joinable())
        thread.join();
}

// ---------------------------------------------------------------------------
// Main thread API
// ---------------------------------------------------------------------------

bool MGamePublisher::start(const SPublishSettings& settings, SString& outError)
{
    if (state.load() == EPublishState::Running)
    {
        outError = "A publish is already running";
        return false;
    }
    if (thread.joinable())
        thread.join();

    auto* app = dynamic_cast<MEditorApplication*>(MApplication::getAppInstance());
    auto* assetManager = dynamic_cast<MAssetManager*>(
        MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
    if (!app || !assetManager || !assetManager->getAssetSource())
    {
        outError = "The editor is not ready to publish";
        return false;
    }

    SJob job;
    job.settings    = settings;
    job.source      = assetManager->getAssetSource();
    job.engineRoot  = SMeteorPaths::engineRoot();
    job.projectRoot = SMeteorPaths::projectRoot();

    const auto& descriptor = app->getProjectManager().getDescriptor();
    job.projectAssetFolders = descriptor.assetFolders;

    // -- Output folder checks ---------------------------------------------------
    if (settings.outputDirectory.empty())
    {
        outError = "Choose an output folder";
        return false;
    }
    fs::path output(settings.outputDirectory.str());
    if (output.is_relative())
        output = job.projectRoot / output;
    output = output.lexically_normal();
    job.settings.outputDirectory = SString(output.generic_string());

    // In the engine folder itself, the development player would find the
    // .manifest and start as the published game.
    if (isSameOrInside(job.engineRoot, output))
    {
        outError = "The output folder can't be the engine folder, or contain it";
        return false;
    }
    if (isSameOrInside(output, job.engineRoot / "meteor_assets"))
    {
        outError = "The output folder can't be inside the engine's meteor_assets folder";
        return false;
    }
    if (isSameOrInside(job.projectRoot, output))
    {
        outError = "The output folder can't be the project folder, or contain it";
        return false;
    }
    for (const auto& folder : job.projectAssetFolders)
    {
        if (isSameOrInside(output, job.projectRoot / folder.str()))
        {
            outError = SString("The output folder can't be inside the asset folder ") + folder;
            return false;
        }
    }

    if (settings.startupScenePath.empty())
    {
        outError = "Choose a startup scene";
        return false;
    }

    // -- Game info and embedded settings (read on the main thread) -------------
    job.gameInfo.name          = settings.gameName.empty() ? descriptor.name : settings.gameName;
    job.gameInfo.id            = descriptor.id;
    job.gameInfo.version       = settings.version;
    job.gameInfo.publisher     = settings.publisher;
    job.gameInfo.engineVersion = descriptor.engineVersion;

    job.embeddedSettings.physicsLayersXml  = MEngineStatics::physicsLayersToXml();
    job.embeddedSettings.engineSettingsXml = MEngineStatics::engineSettingsToXml();
    if (job.embeddedSettings.physicsLayersXml.empty() || job.embeddedSettings.engineSettingsXml.empty())
    {
        outError = "Physics settings are not loaded";
        return false;
    }

    {
        // Player settings: only what players may change. Values from an
        // existing PlayerSettings.xml in the output are kept (other fields in
        // it, from older builds, are dropped).
        MPlayerSettings playerSettings;
        const fs::path existing = output / PLAYER_SETTINGS;
        std::error_code ec;
        if (fs::exists(existing, ec))
            playerSettings.deserialiseFromFile(existing.string());

        pugi::xml_document doc;
        pugi::xml_node root = doc.append_child("root");
        playerSettings.serialiseToNode(root);
        std::ostringstream oss;
        doc.save(oss);
        job.playerSettingsXml = oss.str();
    }

    // -- Start --------------------------------------------------------------------
    cancelFlag = false;
    done       = 0;
    total      = 0;
    {
        std::lock_guard lock(dataMutex);
        currentItem = SString();
        result      = SPublishResult{};
        result.outputDirectory = job.settings.outputDirectory;
        pendingLog.clear();
    }
    state = EPublishState::Running;

    MLOG(SString::format("Publish:: publishing {0} {1} to {2}", job.gameInfo.name, job.gameInfo.version,
                         job.settings.outputDirectory));
    thread = std::thread(&MGamePublisher::run, this, std::move(job));
    return true;
}

void MGamePublisher::cancel()
{
    if (state.load() == EPublishState::Running)
        cancelFlag = true;
}

void MGamePublisher::tick()
{
    std::vector<std::pair<ELogLevel, SString>> lines;
    {
        std::lock_guard lock(dataMutex);
        lines.swap(pendingLog);
    }
    for (const auto& [level, message] : lines)
    {
        switch (level)
        {
            case ELogLevel::Info:    MLOG(message);   break;
            case ELogLevel::Warning: MWARN(message);  break;
            case ELogLevel::Error:   MERROR(message); break;
        }
    }

    if (state.load() != EPublishState::Running && thread.joinable())
        thread.join();
}

void MGamePublisher::acknowledge()
{
    if (state.load() != EPublishState::Running)
        state = EPublishState::Idle;
}

SString MGamePublisher::getCurrentItem() const
{
    std::lock_guard lock(dataMutex);
    return currentItem;
}

SPublishResult MGamePublisher::getResult() const
{
    std::lock_guard lock(dataMutex);
    return result;
}

void MGamePublisher::log(ELogLevel level, const SString& message)
{
    std::lock_guard lock(dataMutex);
    pendingLog.emplace_back(level, SString("Publish:: ") + message);
}

void MGamePublisher::setCurrentItem(const SString& item)
{
    std::lock_guard lock(dataMutex);
    currentItem = item;
}

// ---------------------------------------------------------------------------
// Background job
// ---------------------------------------------------------------------------

void MGamePublisher::run(SJob job)
{
    const fs::path output  = fs::path(job.settings.outputDirectory.str());
    const fs::path staging = output / STAGING_FOLDER;

    SString error;
    bool ok = false;

    try
    {
        std::error_code ec;
        fs::create_directories(output, ec);
        fs::remove_all(staging, ec);
        if (!fs::create_directories(staging, ec) && !fs::exists(staging))
        {
            error = SString("Cannot create ") + SString(staging.generic_string());
        }
        else
        {
            std::vector<SPendingAsset> assets;
            std::vector<SCoreAsset>    coreAssets;
            for (const auto& src : CORE_ASSET_SOURCES)
            {
                const fs::path folder = job.engineRoot / src.folder;
                size_t found = 0;
                for (const auto& entry : fs::directory_iterator(folder, ec))
                {
                    if (!entry.is_regular_file(ec) || entry.path().extension() != src.extension)
                        continue;
                    coreAssets.push_back({ SString(src.prefix) + SString(entry.path().filename().string()), entry.path() });
                    ++found;
                }
                ec.clear();
                if (found == 0)
                    log(ELogLevel::Warning, SString("no engine files found in ") + SString(folder.generic_string()));
            }
            std::sort(coreAssets.begin(), coreAssets.end(),
                      [](const SCoreAsset& a, const SCoreAsset& b) { return a.name < b.name; });

            ok = collectAssets(job, assets, error)
              && writePackage(job, assets, coreAssets, staging, error)
              && copyRuntime(job, staging, error)
              && finalise(job, staging, error);
        }
    }
    catch (const std::exception& e)
    {
        ok    = false;
        error = SString("Unexpected error: ") + SString(e.what());
    }

    std::error_code ec;
    fs::remove_all(staging, ec);

    if (cancelRequested())
    {
        log(ELogLevel::Warning, "cancelled; the output folder was not changed");
        state = EPublishState::Cancelled;
        return;
    }

    if (!ok)
    {
        log(ELogLevel::Error, SString("failed: ") + error);
        {
            std::lock_guard lock(dataMutex);
            result.error = error;
        }
        state = EPublishState::Failed;
        return;
    }

    const SPublishResult summary = getResult();
    log(ELogLevel::Info, SString::format("done - {0} assets, {1} pak(s), {2}",
                                         SString::fromInt(static_cast<int>(summary.assetCount)),
                                         SString::fromInt(static_cast<int>(summary.pakCount)),
                                         formatBytes(summary.totalBytes)));
    state = EPublishState::Succeeded;
}

bool MGamePublisher::collectAssets(const SJob& job, std::vector<SPendingAsset>& out, SString& outError)
{
    setCurrentItem("Collecting assets");

    std::vector<SString> paths;
    job.source->enumerate([&](const SAssetSourceEntry& e)
    {
        if (!e.isDirectory && !MAssetPath::isIgnored(e.path))
            paths.push_back(e.path);
    });

    // The editor only lists meteor_assets/engine_assets/, but engine assets
    // also read files outside it by path - e.g. every sub-shader reads its
    // base source from meteor_assets/shader_base/. Pack the whole engine
    // meteor_assets folder so those reads work in the published game.
    {
        const fs::path engineAssets = job.engineRoot / "meteor_assets";
        std::error_code ec;
        for (fs::recursive_directory_iterator it(engineAssets, fs::directory_options::skip_permission_denied, ec), end;
             !ec && it != end; it.increment(ec))
        {
            if (!it->is_regular_file(ec))
                continue;
            const SString path = MAssetPath::normalize(
                SString("meteor_assets/" + it->path().lexically_relative(engineAssets).generic_string()));
            // Shader include files go in as core assets instead.
            if (!MAssetPath::isIgnored(path) && !path.startsWith(SString(SHADER_UTILS_FOLDER) + "/"))
                paths.push_back(path);
        }
        if (ec)
            log(ELogLevel::Warning, SString("could not list engine assets: ") + SString(ec.message()));
    }

    paths.erase(std::remove_if(paths.begin(), paths.end(), [](const SString& p)
    {
        return p.startsWith(SString(SHADER_UTILS_FOLDER) + "/");
    }), paths.end());
    std::sort(paths.begin(), paths.end());
    paths.erase(std::unique(paths.begin(), paths.end()), paths.end());

    if (paths.empty())
    {
        outError = "The project has no assets";
        return false;
    }

    total = static_cast<uint32_t>(paths.size());
    out.reserve(paths.size());

    std::map<SString, SString> pathByGuid;
    for (const auto& path : paths)
    {
        if (cancelRequested()) return false;

        pugi::xml_document meta;
        bool rewriteMeta = false;
        if (!job.source->hasMeta(path) || !job.source->readMeta(path, meta) || !meta.child(META_TAG))
        {
            // The editor normally creates this when the asset loads; a file
            // added moments ago may not have one yet.
            meta.reset();
            meta.append_child(META_TAG);
            rewriteMeta = true;
        }

        auto tag = meta.child(META_TAG);
        SString guid = tag.attribute(META_ID_ATTR).as_string();
        if (guid.empty())
        {
            guid = SGuid::newGUID();
            if (tag.attribute(META_ID_ATTR))
                tag.attribute(META_ID_ATTR).set_value(guid.c_str());
            else
                tag.append_attribute(META_ID_ATTR).set_value(guid.c_str());
            rewriteMeta = true;
        }

        if (rewriteMeta)
        {
            // Write it back so the project and the build share the GUID.
            if (auto* writable = job.source->asWritable(); writable && writable->writeMeta(path, meta))
                log(ELogLevel::Warning, SString("created missing meta for ") + path);
            else
                log(ELogLevel::Warning, SString("could not save new meta for ") + path +
                                        " - references by GUID to it will not survive the next publish");
        }

        if (const auto it = pathByGuid.find(guid); it != pathByGuid.end())
        {
            outError = SString::format("{0} and {1} have the same GUID ({2}). "
                                       "Delete one of their .meta files and let the editor recreate it.",
                                       it->second, path, guid);
            return false;
        }
        pathByGuid[guid] = path;

        out.push_back(SPendingAsset{ path, guid, serialiseMeta(meta) });
    }

    // The startup scene must be one of the packed assets.
    const SString startup = MAssetPath::normalize(job.settings.startupScenePath);
    if (std::none_of(out.begin(), out.end(), [&](const SPendingAsset& a) { return a.path == startup; }))
    {
        outError = SString("The startup scene is not in the project's assets: ") + startup;
        return false;
    }

    reportOrphanMetas(job, out);
    return true;
}

void MGamePublisher::reportOrphanMetas(const SJob& job, const std::vector<SPendingAsset>& assets)
{
    // Every folder that holds a packed asset. Orphaned metas sit next to
    // where their asset used to be.
    std::set<SString> folders;
    for (const auto& a : assets)
    {
        const SString& p = a.path;
        const size_t slash = p.findLast('/');
        if (slash != SString::npos)
            folders.insert(p.substring(0, slash));
    }

    size_t orphans = 0;
    for (const auto& folder : folders)
    {
        const fs::path disk(job.source->getDiskPath(folder).str());
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(disk, ec))
        {
            if (!entry.is_regular_file(ec) || entry.path().extension() != ".meta")
                continue;

            const SString metaPath = folder + "/" + SString(entry.path().filename().string());
            const SString target = metaPath.substring(0, metaPath.size() - 5);
            if (!job.source->exists(target))
            {
                ++orphans;
                log(ELogLevel::Warning, SString("skipped orphaned meta (its asset no longer exists): ") + metaPath);
            }
        }
    }

    std::lock_guard lock(dataMutex);
    result.orphanMetaCount = orphans;
}

bool MGamePublisher::writePackage(const SJob& job, const std::vector<SPendingAsset>& assets,
                                  const std::vector<SCoreAsset>& coreAssets,
                                  const fs::path& staging, SString& outError)
{
    const SString  buildId    = SGuid::newGUID();
    const uint64_t maxPakSize = static_cast<uint64_t>(std::max<uint32_t>(job.settings.maxPakSizeMB, 1)) * 1024 * 1024;

    SPackagedAssetManifest manifest;
    manifest.setBuildId(buildId);
    manifest.setGameInfo(job.gameInfo);
    manifest.setSettings(job.embeddedSettings);

    std::ofstream pak;
    uint32_t pakIndex   = 0;
    uint64_t pakBytes   = 0;
    uint64_t headerSize = 0;
    uint64_t packed     = 0;

    auto openPak = [&](uint32_t index) -> bool
    {
        if (pak.is_open()) pak.close();
        const fs::path path = staging / pakFileName(index).str();
        pak.open(path, std::ios::binary | std::ios::trunc);
        if (!pak.is_open())
        {
            outError = SString("Cannot create ") + SString(path.generic_string());
            return false;
        }

        SByteWriter header;
        header.u32(PAK_MAGIC);
        header.u32(FORMAT_VERSION);
        header.str(buildId);
        header.u32(index);
        if (!writeAll(pak, header.bytes.data(), header.bytes.size()))
        {
            outError = SString("Cannot write ") + SString(path.generic_string());
            return false;
        }
        headerSize = header.bytes.size();
        pakBytes   = headerSize;
        return true;
    };

    if (!openPak(0))
        return false;

    done = 0;
    std::vector<uint8_t> data;
    for (const auto& asset : assets)
    {
        if (cancelRequested()) return false;
        setCurrentItem(asset.path);

        data.clear();
        if (!job.source->readBytes(asset.path, data))
        {
            outError = SString("Cannot read ") + asset.path;
            return false;
        }

        const uint64_t entrySize = asset.metaBytes.size() + data.size();
        // Roll over to a new pak, unless this one is still empty (an asset
        // bigger than the limit gets a pak of its own).
        if (pakBytes > headerSize && pakBytes + entrySize > maxPakSize)
        {
            if (!openPak(++pakIndex))
                return false;
        }

        SPackagedAssetEntry entry;
        entry.guid     = asset.guid;
        entry.path     = asset.path;
        entry.pakId    = pakIndex;
        entry.offset   = pakBytes;
        entry.metaSize = static_cast<uint32_t>(asset.metaBytes.size());
        entry.dataSize = data.size();

        if (!writeAll(pak, asset.metaBytes.c_str(), asset.metaBytes.size()) ||
            !writeAll(pak, data.data(), data.size()))
        {
            outError = SString::format("Cannot write {0} (is the disk full?)", pakFileName(pakIndex));
            return false;
        }

        pakBytes += entrySize;
        packed   += data.size();
        manifest.addEntry(entry);
        ++done;
    }

    // Core assets: data only, looked up by name.
    for (const auto& core : coreAssets)
    {
        if (cancelRequested()) return false;

        const SString& name = core.name;
        const fs::path& file = core.file;
        setCurrentItem(name);

        std::ifstream in(file, std::ios::binary);
        std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (!in.good() && !in.eof())
        {
            outError = SString("Cannot read ") + SString(file.generic_string());
            return false;
        }

        if (pakBytes > headerSize && pakBytes + bytes.size() > maxPakSize)
        {
            if (!openPak(++pakIndex))
                return false;
        }

        SPackagedCoreAssetEntry entry;
        entry.name   = name;
        entry.pakId  = pakIndex;
        entry.offset = pakBytes;
        entry.size   = bytes.size();
        if (!writeAll(pak, bytes.data(), bytes.size()))
        {
            outError = SString::format("Cannot write {0} (is the disk full?)", pakFileName(pakIndex));
            return false;
        }
        pakBytes += bytes.size();
        manifest.addCoreAsset(entry);
    }

    pak.close();
    if (pak.fail())
    {
        outError = SString::format("Cannot finish writing {0}", pakFileName(pakIndex));
        return false;
    }

    // Startup scene: path and GUID, so it still resolves if moved later.
    const SString startup = MAssetPath::normalize(job.settings.startupScenePath);
    const auto*   scene   = manifest.findByPath(startup);
    manifest.setStartupScene(SStartupScene{ scene ? scene->guid : SString(), startup });
    manifest.setPakCount(pakIndex + 1);

    setCurrentItem("Writing manifest");
    const auto bytes = manifest.serialise();
    std::ofstream out(staging / MANIFEST_FILE_NAME, std::ios::binary | std::ios::trunc);
    if (!out.is_open() || !writeAll(out, bytes.data(), bytes.size()))
    {
        outError = "Cannot write .manifest";
        return false;
    }
    out.close();

    std::lock_guard lock(dataMutex);
    result.assetCount = assets.size();
    result.pakCount   = pakIndex + 1;
    result.totalBytes = packed;
    return true;
}

bool MGamePublisher::copyRuntime(const SJob& job, const fs::path& staging, SString& outError)
{
    if (cancelRequested()) return false;
    setCurrentItem("Copying player");

    std::error_code ec;

    // Player executable, renamed after the game.
    const fs::path player = job.engineRoot / (SString(PLAYER_EXECUTABLE) + EXE_SUFFIX).str();
    if (!fs::exists(player, ec))
    {
        outError = SString("MeteorPlayer was not found next to the editor (") +
                   SString(player.generic_string()) + "). Build the player target first.";
        return false;
    }
    const fs::path exe = staging / (executableName(job.gameInfo.name) + EXE_SUFFIX).str();
    if (!fs::copy_file(player, exe, fs::copy_options::overwrite_existing, ec))
    {
        outError = SString("Cannot copy the player: ") + SString(ec.message());
        return false;
    }

    // Runtime libraries next to the engine executables.
    for (const auto& entry : fs::directory_iterator(job.engineRoot, ec))
    {
        if (!entry.is_regular_file(ec) || entry.path().extension() != LIB_SUFFIX)
            continue;
        if (!fs::copy_file(entry.path(), staging / entry.path().filename(), fs::copy_options::overwrite_existing, ec))
        {
            outError = SString("Cannot copy ") + SString(entry.path().filename().string()) + ": " + SString(ec.message());
            return false;
        }
    }

    // Player-editable settings (resolution, fps, fullscreen).
    {
        std::ofstream settings(staging / PLAYER_SETTINGS, std::ios::trunc);
        settings << job.playerSettingsXml;
        if (!settings)
        {
            outError = "Cannot write PlayerSettings.xml";
            return false;
        }
    }
    return true;
}

bool MGamePublisher::finalise(const SJob& job, const fs::path& staging, SString& outError)
{
    if (cancelRequested()) return false;
    setCurrentItem("Finishing");

    const fs::path output(job.settings.outputDirectory.str());
    std::error_code ec;

    // Remove the previous build's package files, so no stale pak from a
    // bigger earlier build is left behind.
    for (const auto& entry : fs::directory_iterator(output, ec))
    {
        if (!entry.is_regular_file(ec)) continue;
        const auto name = entry.path().filename().string();
        if (name == MANIFEST_FILE_NAME || entry.path().extension() == PAK_EXTENSION)
            fs::remove(entry.path(), ec);
    }

    // Move the new build into place.
    for (const auto& entry : fs::directory_iterator(staging, ec))
    {
        const fs::path to = output / entry.path().filename();
        if (entry.is_directory(ec))
        {
            fs::create_directories(to, ec);
            fs::copy(entry.path(), to, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        }
        else
        {
            fs::remove(to, ec);
            ec.clear();
            fs::rename(entry.path(), to, ec);
            if (ec)   // different volume or locked; fall back to copying
            {
                ec.clear();
                fs::copy_file(entry.path(), to, fs::copy_options::overwrite_existing, ec);
            }
        }

        if (ec)
        {
            outError = SString::format("Cannot write {0} ({1}). Is the game still running?",
                                       SString(to.generic_string()), SString(ec.message()));
            return false;
        }
    }
    return true;
}
