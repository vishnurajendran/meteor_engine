//
// miniaudio_asset_vfs.h
//
// miniaudio virtual file system backed by the active IAssetSource.
//
// Installed on the ma_engine's resource manager (and used for the metadata
// decoder in MMiniAudioClip), so every file miniaudio opens, including the
// streaming reads it does on its job threads, goes through the asset source.
// The same code then plays audio from a project folder or from a package.
//
// The source is looked up on every open via MAssetSources::getActive(), so
// switching projects does not require re-initialising the audio engine.
// Streams that are already open keep their own handle.
//

#ifndef MINIAUDIO_ASSET_VFS_H
#define MINIAUDIO_ASSET_VFS_H

#include "miniaudio.h"

struct SMiniAudioAssetVFS
{
    ma_vfs_callbacks callbacks;   // MUST be the first member: miniaudio casts ma_vfs* to this

    SMiniAudioAssetVFS();

    ma_vfs* get() { return reinterpret_cast<ma_vfs*>(this); }
};

#endif // MINIAUDIO_ASSET_VFS_H
