// hotreloadwatcher.cpp

#include "hotreloadwatcher.h"

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/utils/logger.h"

void MHotReloadWatcher::watchAsset(MAsset* asset)
{
    if (!asset) return;
    const SString path = asset->getPath();
    if (path.empty()) return;

    SWatchEntry entry;
    entry.asset         = asset;
    entry.lastWriteTime = getWriteTime(path);
    watchMap[path]      = entry;
}

void MHotReloadWatcher::unwatchAsset(const SString& path)
{
    watchMap.erase(path);
}

void MHotReloadWatcher::unwatchAll()
{
    watchMap.clear();
}

bool MHotReloadWatcher::isTimeToPoll() const
{
    const double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - lastPollTime).count();
    return elapsed >= POLL_INTERVAL_SECONDS;
}

int MHotReloadWatcher::tick()
{
    if (!isTimeToPoll())
        return 0;

    lastPollTime = std::chrono::steady_clock::now();
    int reloadCount = 0;

    for (auto& [path, entry] : watchMap)
    {
        if (!entry.asset) continue;

        const auto currentWriteTime = getWriteTime(path);

        if (!entry.hasPendingChange && currentWriteTime != entry.lastWriteTime)
        {
            entry.hasPendingChange = true;
            entry.changeDetectedAt = std::chrono::steady_clock::now();
            continue;
        }

        if (!entry.hasPendingChange)
            continue;

        const double timeSinceChange = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - entry.changeDetectedAt).count();

        if (timeSinceChange < DEBOUNCE_SECONDS)
            continue;

        MLOG(STR("HotReloadWatcher:: Reloading ") + path);
        entry.asset->requestReload();
        entry.lastWriteTime    = getWriteTime(path);
        entry.hasPendingChange = false;
        ++reloadCount;
    }

    return reloadCount;
}

std::filesystem::file_time_type MHotReloadWatcher::getWriteTime(const SString& path)
{
    try
    {
        return std::filesystem::last_write_time(std::filesystem::path(path.str()));
    }
    catch (...)
    {
        return {};
    }
}