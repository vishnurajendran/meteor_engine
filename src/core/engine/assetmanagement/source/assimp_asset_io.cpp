//
// assimp_asset_io.cpp
//

#include "core/engine/assetmanagement/source/assimp_asset_io.h"

#include <cstring>

// ---------------------------------------------------------------------------
// Stream
// ---------------------------------------------------------------------------

size_t MAssimpAssetIOStream::Read(void* pvBuffer, size_t pSize, size_t pCount)
{
    if (pSize == 0 || pCount == 0) return 0;

    // Assimp expects the number of whole elements read, like fread().
    const size_t bytes = stream->read(pvBuffer, pSize * pCount);
    return bytes / pSize;
}

aiReturn MAssimpAssetIOStream::Seek(size_t pOffset, aiOrigin pOrigin)
{
    uint64_t target = 0;
    switch (pOrigin)
    {
        case aiOrigin_SET: target = pOffset;                  break;
        case aiOrigin_CUR: target = stream->tell() + pOffset; break;
        case aiOrigin_END: target = stream->size() + pOffset; break;   // same as fseek(SEEK_END)
        default:           return aiReturn_FAILURE;
    }
    return stream->seek(target) ? aiReturn_SUCCESS : aiReturn_FAILURE;
}

size_t MAssimpAssetIOStream::Tell() const     { return static_cast<size_t>(stream->tell()); }
size_t MAssimpAssetIOStream::FileSize() const { return static_cast<size_t>(stream->size()); }

// ---------------------------------------------------------------------------
// IO system
// ---------------------------------------------------------------------------

bool MAssimpAssetIOSystem::Exists(const char* pFile) const
{
    if (!pFile || !*pFile) return false;
    return source->exists(MAssetPath::normalize(SString(pFile)));
}

Assimp::IOStream* MAssimpAssetIOSystem::Open(const char* pFile, const char* pMode)
{
    if (!pFile || !*pFile) return nullptr;

    // Read-only: asset sources are never written through Assimp.
    if (pMode && (std::strchr(pMode, 'w') || std::strchr(pMode, 'a') || std::strchr(pMode, '+')))
        return nullptr;

    auto stream = source->openStream(MAssetPath::normalize(SString(pFile)));
    if (!stream) return nullptr;

    return new MAssimpAssetIOStream(std::move(stream));
}

void MAssimpAssetIOSystem::Close(Assimp::IOStream* pFile)
{
    delete pFile;
}
