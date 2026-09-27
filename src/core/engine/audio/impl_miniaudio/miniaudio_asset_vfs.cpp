//
// miniaudio_asset_vfs.cpp
//
// All callbacks may run on miniaudio job threads. They only touch the
// IAssetStream they were given (one per open file) and the thread-safe
// MAssetSources::getActive().
//

#include "miniaudio_asset_vfs.h"

#include <cstddef>
#include <type_traits>

#include "core/engine/assetmanagement/source/asset_sources.h"

static_assert(std::is_standard_layout_v<SMiniAudioAssetVFS>,
              "SMiniAudioAssetVFS must be standard layout so ma_vfs* can alias it");
static_assert(offsetof(SMiniAudioAssetVFS, callbacks) == 0,
              "ma_vfs_callbacks must be the first member");

namespace
{
IAssetStream* toStream(ma_vfs_file file) { return static_cast<IAssetStream*>(file); }

ma_result vfsOpen(ma_vfs*, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile)
{
    if (pFile == nullptr || pFilePath == nullptr) return MA_INVALID_ARGS;
    *pFile = nullptr;

    if (openMode & MA_OPEN_MODE_WRITE) return MA_ACCESS_DENIED;

    auto stream = MAssetSources::getActive()->openStream(MAssetPath::normalize(SString(pFilePath)));
    if (!stream) return MA_DOES_NOT_EXIST;

    *pFile = stream.release();
    return MA_SUCCESS;
}

ma_result vfsClose(ma_vfs*, ma_vfs_file file)
{
    delete toStream(file);
    return MA_SUCCESS;
}

ma_result vfsRead(ma_vfs*, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead)
{
    if (pBytesRead) *pBytesRead = 0;
    if (file == nullptr) return MA_INVALID_ARGS;

    const size_t got = toStream(file)->read(pDst, sizeInBytes);
    if (pBytesRead) *pBytesRead = got;

    // Matches miniaudio's default VFS: MA_AT_END only when nothing was read.
    return (got == 0 && sizeInBytes > 0) ? MA_AT_END : MA_SUCCESS;
}

ma_result vfsSeek(ma_vfs*, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin)
{
    if (file == nullptr) return MA_INVALID_ARGS;
    auto* s = toStream(file);

    ma_int64 base = 0;
    switch (origin)
    {
        case ma_seek_origin_start:   base = 0;                               break;
        case ma_seek_origin_current: base = static_cast<ma_int64>(s->tell()); break;
        case ma_seek_origin_end:     base = static_cast<ma_int64>(s->size()); break;
        default:                     return MA_INVALID_ARGS;
    }

    const ma_int64 target = base + offset;
    if (target < 0 || target > static_cast<ma_int64>(s->size()))
        return MA_BAD_SEEK;

    return s->seek(static_cast<uint64_t>(target)) ? MA_SUCCESS : MA_BAD_SEEK;
}

ma_result vfsTell(ma_vfs*, ma_vfs_file file, ma_int64* pCursor)
{
    if (file == nullptr || pCursor == nullptr) return MA_INVALID_ARGS;
    *pCursor = static_cast<ma_int64>(toStream(file)->tell());
    return MA_SUCCESS;
}

ma_result vfsInfo(ma_vfs*, ma_vfs_file file, ma_file_info* pInfo)
{
    if (file == nullptr || pInfo == nullptr) return MA_INVALID_ARGS;
    pInfo->sizeInBytes = static_cast<ma_uint64>(toStream(file)->size());
    return MA_SUCCESS;
}
} // namespace

SMiniAudioAssetVFS::SMiniAudioAssetVFS()
{
    callbacks.onOpen  = vfsOpen;
    callbacks.onOpenW = nullptr;   // miniaudio returns MA_NOT_IMPLEMENTED for wide-char opens
    callbacks.onClose = vfsClose;
    callbacks.onRead  = vfsRead;
    callbacks.onWrite = nullptr;   // read-only
    callbacks.onSeek  = vfsSeek;
    callbacks.onTell  = vfsTell;
    callbacks.onInfo  = vfsInfo;
}
