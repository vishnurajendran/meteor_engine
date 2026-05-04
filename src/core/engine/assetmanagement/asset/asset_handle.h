// tassethandle.h
// Suggested location: core/engine/assetmanagement/asset/tassethandle.h
//
// A stable, typed reference to an asset. Stores the asset's GUID rather than
// a raw pointer, so it survives both:
//   - Full MAssetManager::refresh()  (old pointers are deleted and replaced)
//   - In-place requestReload()       (the MAsset* itself stays alive)
//
// ── Migration ────────────────────────────────────────────────────────────────
//   Before:  MTextureAsset* tex = manager->getAsset<MTextureAsset>(path);
//   After:   TAssetHandle<MTextureAsset> tex = manager->getAsset<MTextureAsset>(path);
//
//   You can also wrap a raw pointer you already have:
//            TAssetHandle<MShaderAsset> handle(existingRawPtr);
//
// ── Access ────────────────────────────────────────────────────────────────────
//   tex->getTexture()        — operator-> resolves through the manager
//   tex.get()                — explicit resolve, may return nullptr
//   if (tex) { ... }         — bool cast, false when asset not found
// ─────────────────────────────────────────────────────────────────────────────

#pragma once
#ifndef ASSETHANDLE_H
#define ASSETHANDLE_H

#include <type_traits>
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"

template<typename T>
class TAssetHandle
{
    static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

    SString assetId;

public:
    TAssetHandle() = default;

    explicit TAssetHandle(const SString& id)
        : assetId(id) {}

    // Implicit conversion from raw pointer — convenience for migration.
    // The handle will be null if the pointer has no assetId set yet (i.e.
    // the meta file hasn't been written). That only happens in tests; in
    // normal editor flow every asset gets a GUID on first load.
    TAssetHandle(T* asset) // NOLINT(google-explicit-constructor)
        : assetId(asset ? asset->getAssetId() : SString()) {}

    // Resolves through the manager every call. O(log n) map lookup.
    // Returns nullptr if the asset isn't loaded or the handle is null.
    T* get() const
    {
        if (assetId.empty())
            return nullptr;
        return MAssetManager::getInstance()->getAssetById<T>(assetId);
    }

    T* operator->() const { return get(); }
    T& operator*()  const { return *get(); }

    // True if the handle has a GUID AND the asset is currently loaded.
    explicit operator bool() const { return isValid(); }
    bool isValid() const { return get() != nullptr; }

    // True if the handle was never assigned (no GUID at all).
    bool isNull() const { return assetId.empty(); }

    const SString& getAssetId() const { return assetId; }

    bool operator==(const TAssetHandle<T>& other) const { return assetId == other.assetId; }
    bool operator!=(const TAssetHandle<T>& other) const { return assetId != other.assetId; }
};

#endif // ASSETHANDLE_H