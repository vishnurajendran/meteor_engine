//
// asset_sources.h
//
// Shortcut to the active asset source, which the asset manager subsystem owns
// (IAssetManagerSubsystem::getActiveAssetSource): the open project's folders
// in the editor, the package in a published game. With no asset manager
// registered, a directory source rooted at the working directory.
//
// A separate header on purpose: asset classes and the audio backend need the
// source, and including the manager header from asset.h would create an
// include cycle (asset_handle.h -> asset.h). The lookup lives in the .cpp.
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
};

#endif // ASSET_SOURCES_H
