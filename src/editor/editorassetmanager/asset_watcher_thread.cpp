//
// asset_watcher_thread.cpp
//
// Intended location: src/editor/editorassetmanager/asset_watcher_thread.cpp
//

#include "asset_watcher_thread.h"
#include "core/utils/logger.h"

MAssetWatcherThread::~MAssetWatcherThread()
{
    stop();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MAssetWatcherThread::start(const std::vector<SString>& searchPaths)
{
    if (running.load())
        return;

    searchPaths_  = searchPaths;
    stopRequested = false;
    running       = true;

    workerThread = std::thread(&MAssetWatcherThread::threadFunc, this);
}

void MAssetWatcherThread::stop()
{
    if (!running.load())
        return;

    stopRequested = true;

    if (workerThread.joinable())
        workerThread.join();

    running = false;
}

// ---------------------------------------------------------------------------
// Thread-safe registration (called from main thread)
// ---------------------------------------------------------------------------

void MAssetWatcherThread::watchPath(const SString& path)
{
    std::lock_guard lock(watchMutex);
    if (watchedFiles.count(path.str()))
        return;

    SWatchedFile entry;
    entry.lastWriteTime = getWriteTime(path.str());
    watchedFiles[path.str()] = entry;
}

void MAssetWatcherThread::unwatchPath(const SString& path)
{
    std::lock_guard lock(watchMutex);
    watchedFiles.erase(path.str());
}

void MAssetWatcherThread::unwatchAll()
{
    std::lock_guard lock(watchMutex);
    watchedFiles.clear();
}

void MAssetWatcherThread::addKnownPath(const SString& path)
{
    std::lock_guard lock(knownMutex);
    knownPaths.insert(path.str());
}

void MAssetWatcherThread::removeKnownPath(const SString& path)
{
    std::lock_guard lock(knownMutex);
    knownPaths.erase(path.str());
}

void MAssetWatcherThread::clearKnownPaths()
{
    std::lock_guard lock(knownMutex);
    knownPaths.clear();
}

void MAssetWatcherThread::addKnownDirectory(const SString& path)
{
    std::lock_guard lock(knownMutex);
    knownDirectories.insert(path.str());
}

void MAssetWatcherThread::removeKnownDirectory(const SString& path)
{
    std::lock_guard lock(knownMutex);
    knownDirectories.erase(path.str());
}

void MAssetWatcherThread::clearKnownDirectories()
{
    std::lock_guard lock(knownMutex);
    knownDirectories.clear();
}

// ---------------------------------------------------------------------------
// Event drain (called from main thread once per frame)
// ---------------------------------------------------------------------------

std::vector<SWatchEvent> MAssetWatcherThread::drainEvents()
{
    std::lock_guard lock(eventMutex);
    std::vector<SWatchEvent> result;
    result.swap(pendingEvents);
    return result;
}

void MAssetWatcherThread::requestImmediateScan()
{
    immediateScanRequested.store(true);
}

// ---------------------------------------------------------------------------
// Background thread
// ---------------------------------------------------------------------------

void MAssetWatcherThread::threadFunc()
{
    auto lastPollTime      = std::chrono::steady_clock::now();
    auto lastDeltaScanTime = std::chrono::steady_clock::now();

    while (!stopRequested.load())
    {
        const auto now = std::chrono::steady_clock::now();

        // Poll write times on watched files
        const double sincePoll = std::chrono::duration<double>(now - lastPollTime).count();
        if (sincePoll >= POLL_INTERVAL_SECONDS)
        {
            pollWatchedFiles();
            lastPollTime = now;
        }

        // Scan for new/deleted files and directories
        const double sinceScan = std::chrono::duration<double>(now - lastDeltaScanTime).count();
        const bool forceNow = immediateScanRequested.exchange(false);
        if (forceNow || sinceScan >= DELTA_SCAN_INTERVAL_SECONDS)
        {
            scanForNewAndDeletedFiles();
            lastDeltaScanTime = now;
        }

        // Sleep briefly so this thread does not spin.
        // 100ms granularity is fine -- hot reload does not need sub-second latency.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void MAssetWatcherThread::pollWatchedFiles()
{
    std::lock_guard lock(watchMutex);

    for (auto& [path, entry] : watchedFiles)
    {
        const auto currentWriteTime = getWriteTime(path);

        // Detect initial change
        if (!entry.hasPendingChange && currentWriteTime != entry.lastWriteTime)
        {
            entry.hasPendingChange = true;
            entry.changeDetectedAt = std::chrono::steady_clock::now();
            continue;
        }

        if (!entry.hasPendingChange)
            continue;

        // Wait for debounce period before reporting
        const double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - entry.changeDetectedAt).count();

        if (elapsed < DEBOUNCE_SECONDS)
            continue;

        // Change is debounced -- push event for main thread
        entry.lastWriteTime    = getWriteTime(path);
        entry.hasPendingChange = false;

        {
            std::lock_guard eLock(eventMutex);
            pendingEvents.push_back({EWatchEvent::Modified, SString(path)});
        }
    }
}

void MAssetWatcherThread::scanForNewAndDeletedFiles()
{
    // Snapshot known state so locks are held briefly.
    std::set<std::string> knownPathsSnapshot;
    std::set<std::string> knownDirsSnapshot;
    {
        std::lock_guard lock(knownMutex);
        knownPathsSnapshot = knownPaths;
        knownDirsSnapshot  = knownDirectories;
    }

    std::set<std::string> diskPaths;
    std::set<std::string> diskDirs;

    // Walk all search paths
    for (const auto& searchPath : searchPaths_)
    {
        std::filesystem::path dir(searchPath.str());
        if (!std::filesystem::exists(dir))
            continue;

        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
            {
                if (stopRequested.load())
                    return;

                if (entry.is_directory())
                {
                    std::string dirStr = entry.path().string();
                    for (char& c : dirStr)
                        if (c == '\\') c = '/';
                    diskDirs.insert(dirStr);

                    if (knownDirsSnapshot.find(dirStr) == knownDirsSnapshot.end())
                    {
                        std::lock_guard eLock(eventMutex);
                        pendingEvents.push_back({EWatchEvent::NewDirectory, SString(dirStr)});
                    }
                    continue;
                }

                if (!entry.is_regular_file())
                    continue;

                std::string filename = entry.path().filename().string();
                if (!filename.empty() && filename[0] == '~')
                    continue;

                std::string path = entry.path().string();
                for (char& c : path)
                    if (c == '\\') c = '/';

                if (entry.path().extension().string() == ".meta")
                    continue;

                diskPaths.insert(path);

                if (knownPathsSnapshot.find(path) == knownPathsSnapshot.end())
                {
                    std::lock_guard eLock(eventMutex);
                    pendingEvents.push_back({EWatchEvent::NewFile, SString(path)});
                }
            }
        }
        catch (const std::filesystem::filesystem_error&)
        {
            // Directory may have been deleted mid-scan -- ignore.
        }
    }

    // Detect deleted files
    for (const auto& known : knownPathsSnapshot)
    {
        if (stopRequested.load())
            return;

        if (diskPaths.find(known) == diskPaths.end())
        {
            std::lock_guard eLock(eventMutex);
            pendingEvents.push_back({EWatchEvent::Deleted, SString(known)});
        }
    }

    // Detect deleted directories
    for (const auto& known : knownDirsSnapshot)
    {
        if (stopRequested.load())
            return;

        if (diskDirs.find(known) == diskDirs.end())
        {
            std::lock_guard eLock(eventMutex);
            pendingEvents.push_back({EWatchEvent::DeletedDirectory, SString(known)});
        }
    }
}

std::filesystem::file_time_type MAssetWatcherThread::getWriteTime(const std::string& path)
{
    try
    {
        return std::filesystem::last_write_time(std::filesystem::path(path));
    }
    catch (...)
    {
        return {};
    }
}