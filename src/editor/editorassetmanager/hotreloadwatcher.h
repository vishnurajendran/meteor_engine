// hotreloadwatcher.h
// Suggested location: alongside editorassetmanager.h (editor-only)

#pragma once
#ifndef HOTRELOADWATCHER_H
#define HOTRELOADWATCHER_H

#include <chrono>
#include <filesystem>
#include <map>

#include "../../core/object/object.h"

class MAsset;

struct SWatchEntry
{
    MAsset*                               asset               = nullptr;
    std::filesystem::file_time_type       lastWriteTime       = {};
    std::chrono::steady_clock::time_point changeDetectedAt    = {};
    bool                                  hasPendingChange    = false;
};

class MHotReloadWatcher
{
public:
    static constexpr double POLL_INTERVAL_SECONDS = 1.0;
    static constexpr double DEBOUNCE_SECONDS      = 0.5;

    void watchAsset(MAsset* asset);
    void unwatchAsset(const SString& path);   // call when an asset is deleted
    void unwatchAll();

    // Returns the number of assets reloaded this tick.
    int tick();

private:
    std::map<SString, SWatchEntry>        watchMap;
    std::chrono::steady_clock::time_point lastPollTime = std::chrono::steady_clock::now();

    bool isTimeToPoll() const;
    static std::filesystem::file_time_type getWriteTime(const SString& path);
};

#endif // HOTRELOADWATCHER_H