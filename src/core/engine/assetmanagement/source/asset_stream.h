//
// asset_stream.h
//
// Seekable, read-only byte stream handed out by an IAssetSource.
//
// Every stream owns its own cursor (and, for the directory source, its own
// file handle), so separate streams can be used from separate threads at the
// same time. That matters for miniaudio, which streams audio on its own job
// threads. A single stream instance is NOT thread-safe.
//

#ifndef ASSET_STREAM_H
#define ASSET_STREAM_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

class IAssetStream
{
public:
    virtual ~IAssetStream() = default;

    // Reads up to `bytes` bytes into dst. Returns the number actually read;
    // 0 means end of stream (or an error).
    virtual size_t   read(void* dst, size_t bytes) = 0;

    // Absolute seek. Returns false if offset > size().
    virtual bool     seek(uint64_t offset) = 0;

    virtual uint64_t tell() const = 0;
    virtual uint64_t size() const = 0;

    // Convenience: read the rest of the stream from the current cursor.
    bool readAll(std::vector<uint8_t>& out)
    {
        const uint64_t remaining = size() - tell();
        out.resize(static_cast<size_t>(remaining));
        if (remaining == 0) return true;
        return read(out.data(), out.size()) == out.size();
    }
};

// ---------------------------------------------------------------------------
// Stream over a block of memory. Used by the package source (views into the
// mapped package) and by anything that already has the bytes in memory.
//
// When constructed with `owned`, the stream keeps the buffer alive itself.
// When constructed with a raw pointer, the caller guarantees the memory
// outlives the stream.
// ---------------------------------------------------------------------------

class MMemoryAssetStream final : public IAssetStream
{
public:
    MMemoryAssetStream(const uint8_t* data, uint64_t size)
        : dataPtr(data), dataSize(size) {}

    explicit MMemoryAssetStream(std::shared_ptr<const std::vector<uint8_t>> owned)
        : ownedData(std::move(owned))
    {
        dataPtr  = ownedData ? ownedData->data() : nullptr;
        dataSize = ownedData ? ownedData->size() : 0;
    }

    size_t read(void* dst, size_t bytes) override
    {
        const uint64_t remaining = dataSize - cursor;
        const size_t   n = static_cast<size_t>(std::min<uint64_t>(bytes, remaining));
        if (n > 0)
        {
            std::memcpy(dst, dataPtr + cursor, n);
            cursor += n;
        }
        return n;
    }

    bool seek(uint64_t offset) override
    {
        if (offset > dataSize) return false;
        cursor = offset;
        return true;
    }

    uint64_t tell() const override { return cursor; }
    uint64_t size() const override { return dataSize; }

private:
    std::shared_ptr<const std::vector<uint8_t>> ownedData;
    const uint8_t* dataPtr  = nullptr;
    uint64_t       dataSize = 0;
    uint64_t       cursor   = 0;
};

#endif // ASSET_STREAM_H
