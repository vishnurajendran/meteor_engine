// thumbnailcache.h
// Suggested location: editor/editorassetmanager/thumbnails/

#pragma once
#ifndef THUMBNAIL_CACHE_H
#define THUMBNAIL_CACHE_H

#include <filesystem>
#include <map>
#include "SFML/Graphics/Texture.hpp"
#include "core/object/object.h"

// Owns sf::Texture* objects keyed by asset GUID.
// Thumbnails are persisted to CACHE_DIR so they survive editor restarts
// without needing to be re-rendered.
//
// ── Lifetime rules ────────────────────────────────────────────────────────────
//
//   store()    — saves to memory AND writes a PNG to CACHE_DIR/<assetId>.png
//   get()      — returns from memory; on first access loads from disk if the
//                PNG exists (lazy load), so no startup scan is needed.
//   has()      — true when the texture is in memory OR the disk file exists.
//                Used by requestThumbnail() to avoid re-queuing cached work.
//   evict()    — removes from memory AND deletes the disk file.
//                Call this on hot-reload so the stale thumbnail is regenerated.
//   evictAll() — clears memory only.  Disk files are preserved across full
//                refresh() calls because GUIDs are stable — the same asset
//                has the same ID even after a rescan.
//
// Thread-safety: single-threaded (editor main thread only).
// ─────────────────────────────────────────────────────────────────────────────
class MThumbnailCache
{
public:
    // All cache PNGs live under this directory (created automatically).
    static constexpr const char* CACHE_DIR = ".engine_data/thumbnail_cache";

    ~MThumbnailCache();

    // Store a newly-generated thumbnail in memory and save it to disk.
    // Takes ownership of the texture pointer.
    void store(const SString& assetId, sf::Texture* texture);

    // Return the texture for assetId.
    // If not in memory, attempts to load from the disk cache.
    // Returns nullptr if the thumbnail has not been generated yet.
    sf::Texture* get(const SString& assetId);

    // True if in memory OR a disk file exists.
    bool has(const SString& assetId) const;

    // Remove from memory and delete the disk file.
    // Called when an asset is hot-reloaded or deleted so the stale
    // thumbnail is regenerated on next access.
    void evict(const SString& assetId);

    // Clear memory only — disk files are kept so they can be loaded again
    // after the next full refresh() without re-rendering.
    void evictAll();

private:
    // Full path to the PNG for a given assetId.
    static std::filesystem::path diskPath(const SString& assetId);

    // Create CACHE_DIR if it does not already exist.
    static void ensureCacheDir();

    std::map<SString, sf::Texture*> memCache;
};

#endif // THUMBNAIL_CACHE_H