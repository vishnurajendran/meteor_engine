// thumbnailcache.cpp

#include "thumbnail_cache.h"
#include <filesystem>
#include "SFML/Graphics/Image.hpp"
#include "core/utils/logger.h"

namespace fs = std::filesystem;

// ── Helpers ───────────────────────────────────────────────────────────────────

fs::path MThumbnailCache::diskPath(const SString& assetId)
{
    return fs::path(CACHE_DIR) / (assetId.str() + ".png");
}

void MThumbnailCache::ensureCacheDir()
{
    std::error_code ec;
    fs::create_directories(CACHE_DIR, ec);
    if (ec)
        MWARN("MThumbnailCache: could not create cache dir: " + SString(ec.message().c_str()));
}

// ── Lifetime ──────────────────────────────────────────────────────────────────

MThumbnailCache::~MThumbnailCache()
{
    // evictAll() clears memory only — disk files are intentionally kept.
    evictAll();
}

// ── store ─────────────────────────────────────────────────────────────────────

void MThumbnailCache::store(const SString& assetId, sf::Texture* texture)
{
    // Overwrite any existing memory entry.
    auto it = memCache.find(assetId);
    if (it != memCache.end())
    {
        delete it->second;
        it->second = texture;
    }
    else
    {
        memCache[assetId] = texture;
    }

    // Persist to disk.
    if (texture)
    {
        ensureCacheDir();
        sf::Image img = texture->copyToImage();
        const fs::path p = diskPath(assetId);
        if (!img.saveToFile(p))
            MWARN("MThumbnailCache: failed to save thumbnail to " + SString(p.string().c_str()));
    }
}

// ── get ───────────────────────────────────────────────────────────────────────

sf::Texture* MThumbnailCache::get(const SString& assetId)
{
    // Fast path: already in memory.
    auto it = memCache.find(assetId);
    if (it != memCache.end())
        return it->second;

    // Slow path: try to load from disk (happens once per asset per session).
    const fs::path p = diskPath(assetId);
    if (!fs::exists(p))
        return nullptr;

    auto* tex = new sf::Texture();
    if (!tex->loadFromFile(p))
    {
        delete tex;
        // Disk file is corrupt or unreadable — remove it so it gets regenerated.
        std::error_code ec;
        fs::remove(p, ec);
        return nullptr;
    }

    // Promote to memory cache so subsequent calls are fast.
    memCache[assetId] = tex;
    return tex;
}

// ── has ───────────────────────────────────────────────────────────────────────

bool MThumbnailCache::has(const SString& assetId) const
{
    if (memCache.contains(assetId))
        return true;

    // Check disk without loading — used by requestThumbnail() to avoid
    // queueing assets whose PNG already exists.
    return fs::exists(diskPath(assetId));
}

// ── evict ─────────────────────────────────────────────────────────────────────

void MThumbnailCache::evict(const SString& assetId)
{
    // Memory
    auto it = memCache.find(assetId);
    if (it != memCache.end())
    {
        delete it->second;
        memCache.erase(it);
    }

    // Disk — asset content changed so the cached PNG is stale.
    std::error_code ec;
    fs::remove(diskPath(assetId), ec);
}

// ── evictAll ──────────────────────────────────────────────────────────────────

void MThumbnailCache::evictAll()
{
    // Clear GPU textures from memory.
    for (auto& [id, tex] : memCache)
        delete tex;
    memCache.clear();

    // Disk files are intentionally NOT deleted here.  After a full refresh()
    // the same asset GUIDs are re-discovered, and their on-disk thumbnails
    // will be loaded by get() on first access — no re-render needed.
}