//
// asset_watcher_thread.cpp
//
// Intended location: src/editor/editorassetmanager/asset_watcher_thread.cpp
//

#include "asset_watcher_thread.h"
#include "core/engine/assetmanagement/source/directory_asset_source.h"
#include "core/utils/logger.h"

MAssetWatcherThread::~MAssetWatcherThread()
{
    stop();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MAssetWatcherThread::setSource(std::shared_ptr<const MDirectoryAssetSource> source)
{
    std::vector<std::shared_ptr<const MDirectoryAssetSource>> list;
    if (source) list.push_back(std::move(source));
    setSources(std::move(list));
}

void MAssetWatcherThread::setSources(std::vector<std::shared_ptr<const MDirectoryAssetSource>> sources)
{
    if (running.load())
    {
        MWARN("MAssetWatcherThread:: setSources ignored while running; call stop() first");
        return;
    }
    sources_ = std::move(sources);
}

const MDirectoryAssetSource* MAssetWatcherThread::sourceFor(const std::string& assetPath) const
{
    for (const auto& src : sources_)
    {
        for (const auto& searchPath : src->getSearchPaths())
        {
            std::string prefix = MAssetPath::normalize(searchPath).str();
            if (prefix.empty()) continue;
            if (prefix.back() != '/') prefix.push_back('/');
            if (assetPath.compare(0, prefix.size(), prefix) == 0)
                return src.get();
        }
    }
    return sources_.empty() ? nullptr : sources_.front().get();
}

void MAssetWatcherThread::start()
{
    if (running.load() || sources_.empty())
        return;

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

        // File is gone (or unreadable): the delta scan reports it as Deleted.
        // Reporting Modified too would make the editor try to reload a
        // missing file.
        if (currentWriteTime == std::filesystem::file_time_type{})
        {
            entry.hasPendingChange = false;
            continue;
        }

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
    if (sources_.empty())
        return;

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
    std::vector<SWatchEvent> found;

    // Walk all search paths of every source
    for (const auto& source : sources_)
    for (const auto& searchPath : source->getSearchPaths())
    {
        const std::filesystem::path dir = source->resolve(searchPath);
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec))
            continue;

        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
            {
                if (stopRequested.load())
                    return;

                const SString assetPath = source->toAssetPath(entry.path());
                const std::string& key  = assetPath.str();

                if (entry.is_directory())
                {
                    diskDirs.insert(key);
                    if (!knownDirsSnapshot.contains(key))
                        found.push_back({EWatchEvent::NewDirectory, assetPath});
                    continue;
                }

                if (!entry.is_regular_file())
                    continue;

                if (MAssetPath::isIgnored(assetPath))   // .meta and ~files
                    continue;

                diskPaths.insert(key);
                if (!knownPathsSnapshot.contains(key))
                    found.push_back({EWatchEvent::NewFile, assetPath});
            }
        }
        catch (const std::filesystem::filesystem_error&)
        {
            // Directory may have been deleted mid-scan -- skip deletion
            // detection this round so we don't report false deletes.
            return;
        }
    }

    // Detect deleted files
    for (const auto& known : knownPathsSnapshot)
    {
        if (stopRequested.load())
            return;

        if (!diskPaths.contains(known))
            found.push_back({EWatchEvent::Deleted, SString(known)});
    }

    // Detect deleted directories
    for (const auto& known : knownDirsSnapshot)
    {
        if (stopRequested.load())
            return;

        if (!diskDirs.contains(known))
            found.push_back({EWatchEvent::DeletedDirectory, SString(known)});
    }

    if (!found.empty())
    {
        std::lock_guard eLock(eventMutex);
        pendingEvents.insert(pendingEvents.end(), found.begin(), found.end());
    }
}

std::filesystem::file_time_type MAssetWatcherThread::getWriteTime(const std::string& assetPath) const
{
    const MDirectoryAssetSource* source = sourceFor(assetPath);
    if (!source)
        return {};

    std::error_code ec;
    const auto t = std::filesystem::last_write_time(source->resolve(SString(assetPath)), ec);
    return ec ? std::filesystem::file_time_type{} : t;
}
