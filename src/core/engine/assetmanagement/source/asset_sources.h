//
// asset_sources.h
//
// Holds the process-wide active asset source.
//
// The editor sets an MDirectoryAssetSource for the open project; player
// builds will set the package source. Until something sets one, getActive()
// lazily creates a directory source rooted at the working directory, which
// matches how FileIO resolved relative asset paths before this existed.
//
// Kept separate from IAssetManagerSubsystem on purpose: asset classes and the
// audio backend need the source, and including the manager header from
// asset.h would create an include cycle (asset_handle.h -> asset.h).
//
// Thread-safe. getActive() returns a shared_ptr so a caller on another thread
// (e.g. a miniaudio job) keeps the source alive even if the main thread swaps
// it mid-call. Swap sources only when no asset loads are in flight (project
// open/close); streams already open keep working either way.
//

#ifndef ASSET_SOURCES_H
#define ASSET_SOURCES_H

#include <memory>

#include "core/engine/assetmanagement/source/asset_source.h"

class MAssetSources
{
public:
    // Never returns nullptr.
    static std::shared_ptr<IAssetSource> getActive();

    // Returns nullptr when the active source is read-only (player / package).
    // The returned pointer shares ownership with the active source.
    static std::shared_ptr<IWritableAssetSource> getWritable();

    static void setActive(std::shared_ptr<IAssetSource> source);
};

#endif // ASSET_SOURCES_H
