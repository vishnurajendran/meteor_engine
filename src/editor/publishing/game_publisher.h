//
// game_publisher.h
//
// Builds a published game from the open project:
//
//   <output>/<Game>.exe        the player, renamed
//   <output>/*.dll             the player's runtime libraries
//   <output>/.manifest         game info, startup scene, physics settings,
//                              and the GUID -> asset / path -> GUID tables
//   <output>/00.pak, ...       every project and engine asset with its meta
//   <output>/PlayerSettings.xml  resolution, fps, fullscreen: the only
//                              settings players may edit. Values already in
//                              the output's copy are kept.
//
// Project settings (rendering, physics, physics layers) are embedded in the
// .manifest and cannot be changed by players. Engine files the player needs
// by name - shader include files and Lua engine modules - are stored in the
// paks as core assets (no meta, no GUID).
//
// Packs exactly what the editor's asset source lists, so a publish contains
// the same assets the editor loaded. Orphaned .meta files (left behind by
// deleted assets) are never packed and are reported as warnings.
//
// The work runs on a background thread. Everything is written to a staging
// folder first; the output folder is only touched once the build succeeded,
// so cancelling or failing leaves the previous build intact.
//
// While a job runs, isPublishing() is true and the editor blocks authoring:
// shortcuts, play mode, hot reload and script refresh are paused, and the
// progress popup is modal.
//

#ifndef GAME_PUBLISHER_H
#define GAME_PUBLISHER_H

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "core/package/package_format.h"
#include "core/utils/sstring.h"

class IAssetSource;

struct SPublishSettings
{
    SString  outputDirectory;
    SString  gameName;
    SString  version;
    SString  publisher;
    SString  startupScenePath;
    uint32_t maxPakSizeMB = 512;
};

enum class EPublishState
{
    Idle,
    Running,
    Succeeded,
    Failed,
    Cancelled,
};

struct SPublishResult
{
    size_t   assetCount      = 0;
    uint32_t pakCount        = 0;
    uint64_t totalBytes      = 0;   // asset bytes packed
    size_t   orphanMetaCount = 0;
    SString  outputDirectory;
    SString  error;                 // set when the state is Failed
};

class MGamePublisher
{
public:
    static MGamePublisher& get();

    // True while a publish job is running. The editor uses this to block
    // anything that could change the project mid-publish.
    static bool isPublishing();

    ~MGamePublisher();

    // Main thread. Captures what the job needs from the editor (asset
    // source, settings, project info) and starts the background thread.
    bool start(const SPublishSettings& settings, SString& outError);

    // Asks the job to stop. It cleans up its staging folder and ends in
    // EPublishState::Cancelled.
    void cancel();

    // Main thread, once per frame: prints queued log lines and joins the
    // thread when the job has finished.
    void tick();

    // Clears a finished job's result so a new one can start.
    void acknowledge();

    [[nodiscard]] EPublishState getState() const { return state.load(); }
    [[nodiscard]] uint32_t      getDone()  const { return done.load(); }
    [[nodiscard]] uint32_t      getTotal() const { return total.load(); }
    [[nodiscard]] SString       getCurrentItem() const;
    [[nodiscard]] SPublishResult getResult() const;

private:
    MGamePublisher() = default;

    struct SJob
    {
        SPublishSettings                 settings;
        std::shared_ptr<IAssetSource>    source;
        std::filesystem::path            engineRoot;
        std::filesystem::path            projectRoot;
        std::vector<SString>             projectAssetFolders;
        SGameInfo                        gameInfo;
        SEmbeddedSettings                embeddedSettings;
        SString                          playerSettingsXml;
    };

    struct SCoreAsset
    {
        SString               name;   // e.g. "lua_modules/Behaviour.lua"
        std::filesystem::path file;
    };

    struct SPendingAsset
    {
        SString path;
        SString guid;
        SString metaBytes;
    };

    enum class ELogLevel { Info, Warning, Error };

    void run(SJob job);
    bool collectAssets(const SJob& job, std::vector<SPendingAsset>& out, SString& outError);
    void reportOrphanMetas(const SJob& job, const std::vector<SPendingAsset>& assets);
    bool writePackage(const SJob& job, const std::vector<SPendingAsset>& assets,
                      const std::vector<SCoreAsset>& coreAssets,
                      const std::filesystem::path& staging, SString& outError);
    bool copyRuntime(const SJob& job, const std::filesystem::path& staging, SString& outError);
    bool finalise(const SJob& job, const std::filesystem::path& staging, SString& outError);

    void log(ELogLevel level, const SString& message);
    void setCurrentItem(const SString& item);
    [[nodiscard]] bool cancelRequested() const { return cancelFlag.load(); }

    std::thread                thread;
    std::atomic<EPublishState> state { EPublishState::Idle };
    std::atomic<bool>          cancelFlag { false };
    std::atomic<uint32_t>      done  { 0 };
    std::atomic<uint32_t>      total { 0 };

    mutable std::mutex                          dataMutex;   // guards everything below
    SString                                     currentItem;
    SPublishResult                              result;
    std::vector<std::pair<ELogLevel, SString>>  pendingLog;
};

#endif // GAME_PUBLISHER_H
