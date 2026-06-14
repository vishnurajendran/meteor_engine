//
// asset_watcher_thread.h
//
// Runs filesystem polling on a background thread so that stat() and
// directory-walk syscalls never block the main/render thread.
//
// Events produced:
//   Modified         -- a watched file's write time changed (debounced)
//   NewFile          -- a file on disk is not in the known-paths set
//   Deleted          -- a known file path no longer exists on disk
//   NewDirectory     -- a directory on disk is not in the known-dirs set
//   DeletedDirectory -- a known directory no longer exists on disk
//
// The main thread calls drainEvents() once per frame and handles them.
//
// Intended location: src/editor/editorassetmanager/asset_watcher_thread.h
//

#pragma once
#ifndef ASSET_WATCHER_THREAD_H
#define ASSET_WATCHER_THREAD_H

#include <atomic>
#include <chrono>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "core/object/object.h"

enum class EWatchEvent
{
    Modified,
    NewFile,
    Deleted,
    NewDirectory,
    DeletedDirectory,
};

struct SWatchEvent
{
    EWatchEvent type;
    SString     path;
};

class MAssetWatcherThread
{
public:
    // How often the background thread polls file write-times.
    static constexpr double POLL_INTERVAL_SECONDS       = 1.0;

    // How long after a change is first detected before it is reported,
    // so that partial writes from external editors are not picked up.
    static constexpr double DEBOUNCE_SECONDS            = 0.5;

    // How often the background thread scans directories for new/deleted files.
    static constexpr double DELTA_SCAN_INTERVAL_SECONDS = 1.0;

    MAssetWatcherThread() = default;
    ~MAssetWatcherThread();

    // Start the background thread. Call once after assets are loaded.
    void start(const std::vector<SString>& searchPaths);

    // Stop the background thread. Blocks until the thread joins.
    void stop();

    // -- File watch registration (thread-safe) ---------------------------------

    void watchPath(const SString& path);
    void unwatchPath(const SString& path);
    void unwatchAll();

    // -- Known-path tracking for delta scan (thread-safe) ----------------------

    void addKnownPath(const SString& path);
    void removeKnownPath(const SString& path);
    void clearKnownPaths();

    // -- Known-directory tracking for delta scan (thread-safe) -----------------

    void addKnownDirectory(const SString& path);
    void removeKnownDirectory(const SString& path);
    void clearKnownDirectories();

    // -- Event drain (main thread, once per frame) -----------------------------

    std::vector<SWatchEvent> drainEvents();

    // Force the background thread to run a delta scan on its next iteration,
    // ignoring the normal interval timer. Use after creating a new asset file
    // so it is picked up quickly.
    void requestImmediateScan();

    bool isRunning() const { return running.load(); }

private:
    void threadFunc();
    void pollWatchedFiles();
    void scanForNewAndDeletedFiles();

    static std::filesystem::file_time_type getWriteTime(const std::string& path);

private:
    std::thread             workerThread;
    std::atomic<bool>       running{false};
    std::atomic<bool>       stopRequested{false};
    std::atomic<bool>       immediateScanRequested{false};

    // -- Watched file state (protected by watchMutex) --------------------------
    mutable std::mutex      watchMutex;

    struct SWatchedFile
    {
        std::filesystem::file_time_type       lastWriteTime    = {};
        std::chrono::steady_clock::time_point changeDetectedAt = {};
        bool                                  hasPendingChange = false;
    };
    std::map<std::string, SWatchedFile> watchedFiles;

    // -- Known paths for delta scan (protected by knownMutex) ------------------
    mutable std::mutex      knownMutex;
    std::set<std::string>   knownPaths;
    std::set<std::string>   knownDirectories;
    std::vector<SString>    searchPaths_;

    // -- Output event queue (protected by eventMutex) --------------------------
    mutable std::mutex      eventMutex;
    std::vector<SWatchEvent> pendingEvents;
};

#endif // ASSET_WATCHER_THREAD_H