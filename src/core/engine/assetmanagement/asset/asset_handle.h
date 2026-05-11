#pragma once
#ifndef ASSETHANDLE_H
#define ASSETHANDLE_H

#include <type_traits>
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"

/// TAssetHandle acts as a wrapper over Assets instances when referenced by systems.
/// this wrapper ensures the assets remain valid for use even after a reload and the asset
/// pointer changes.
template<typename T>
class TAssetHandle
{
    static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");
    SString assetId;

public:
    TAssetHandle() = default;

    explicit TAssetHandle(const SString& id)
        : assetId(id) {}

    TAssetHandle(T* asset)
        : assetId(asset ? asset->getAssetId() : SString()) {}

    /// Resolves through the manager every call. O(log n) map lookup.
    /// Returns nullptr if the asset isn't loaded or the handle is null.
    T* get() const
    {
        if (assetId.empty())
            return nullptr;
        return MAssetManager::getInstance()->getAssetById<T>(assetId);
    }

    T* operator->() const { return get(); }
    T& operator*()  const { return *get(); }

    explicit operator bool() const { return isValid(); }

    /// True if the handle has a GUID AND the asset is currently loaded.
    bool isValid() const { return get() != nullptr; }

    /// True if the handle was never assigned (no GUID at all).
    bool isNull() const { return assetId.empty(); }

    /// Returns the asset id held by handle.
    const SString& getAssetId() const { return assetId; }

    bool operator==(const TAssetHandle<T>& other) const { return assetId == other.assetId; }
    bool operator!=(const TAssetHandle<T>& other) const { return assetId != other.assetId; }
};

#endif // ASSETHANDLE_H