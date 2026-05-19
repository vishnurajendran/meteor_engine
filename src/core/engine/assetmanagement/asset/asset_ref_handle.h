//
// Created by ssj5v on 19-05-2026.
//

#ifndef ASSET_REF_HANDLE_H
#define ASSET_REF_HANDLE_H

#include <type_traits>
#include "asset.h"
#include "asset_handle.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"

// TAssetRef<T>
//
// A serializable, typed reference to an asset. Stores both the asset's GUID
// and its file path so it can be persisted to XML (via the Field system) and
// lazily resolved at runtime.
//
// Resolution order:
//   1. Try GUID through MAssetManager::getAssetById<T>().
//   2. If that fails and a path is present, fall back to
//      MAssetManager::getAsset<T>(path).
//   3. On a successful path-based fallback, backfill the GUID so subsequent
//      resolves take the fast path.
//
// This ensures that post-copy directories and older scene files that only
// contain asset paths keep working, while new saves always emit the GUID
// for rename-safe references.
//
// Usage in an entity:
//
//   DECLARE_FIELD(meshAsset, TAssetRef<MStaticMeshAsset>, {})
//
//   // runtime access
//   if (auto* mesh = meshAsset.get().resolve())
//       mesh->getMeshes();
//
//   // assignment from a handle or raw pointer
//   meshAsset = someHandle;
//   meshAsset = someRawPtr;
//

template <typename T>
class TAssetRef
{
    static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

public:
    // -- construction --------------------------------------------------------

    TAssetRef() = default;

    // Construct from an asset ID string (the GUID from the .meta file).
    explicit TAssetRef(const SString& assetId)
        : m_assetId(assetId)
    {}

    // Construct with both ID and path explicitly.
    TAssetRef(const SString& assetId, const SString& path)
        : m_assetId(assetId)
        , m_path(path)
    {}

    // Construct from an existing handle. Captures both GUID and path if the
    // asset is currently loaded.
    TAssetRef(const TAssetHandle<T>& handle) // NOLINT(google-explicit-constructor)
        : m_assetId(handle.getAssetId())
    {
        if (T* asset = handle.get())
            m_path = asset->getPath();
    }

    // Construct from a raw pointer. Reads GUID and path off the asset.
    TAssetRef(T* asset) // NOLINT(google-explicit-constructor)
    {
        if (asset)
        {
            m_assetId = asset->getAssetId();
            m_path    = asset->getPath();
        }
    }

    ~TAssetRef() = default;

    // -- assignment ----------------------------------------------------------

    TAssetRef& operator=(const TAssetHandle<T>& handle)
    {
        m_assetId = handle.getAssetId();
        m_path    = SString{};
        if (T* asset = handle.get())
            m_path = asset->getPath();
        return *this;
    }

    TAssetRef& operator=(T* asset)
    {
        if (asset)
        {
            m_assetId = asset->getAssetId();
            m_path    = asset->getPath();
        }
        else
        {
            m_assetId = SString{};
            m_path    = SString{};
        }
        return *this;
    }

    // -- resolution ----------------------------------------------------------

    // Returns a TAssetHandle built from the current GUID.
    // Does NOT include the path fallback. Use resolve() for full resolution.
    TAssetHandle<T> getHandle() const
    {
        const auto am = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
        return am->getAssetById<T>(m_assetId);
    }

    // Full resolution with fallback.
    // Tries GUID first, then path. Backfills the GUID on a successful
    // path-based resolve so subsequent calls take the fast path.
    // O(log n) map lookup per attempt.
    T* resolve() const
    {
        auto* manager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
        if (!manager)
            return nullptr;

        // Fast path: resolve by GUID.
        if (!m_assetId.empty())
        {
            auto handle = manager->getAssetById<T>(m_assetId);
            if (T* asset = handle.get())
                return asset;
        }

        // Fallback: resolve by file path.
        if (!m_path.empty())
        {
            auto handle = manager->getAsset<T>(m_path);
            if (T* asset = handle.get())
            {
                // Backfill the GUID so future resolves skip the fallback.
                m_assetId = asset->getAssetId();
                return asset;
            }
        }

        return nullptr;
    }

    // -- pointer-like access -------------------------------------------------

    T* operator->() const { return resolve(); }
    T& operator*()  const { return *resolve(); }

    // -- validity ------------------------------------------------------------

    // True if the reference can resolve to a loaded asset right now.
    explicit operator bool() const { return resolve() != nullptr; }
    bool isValid() const { return resolve() != nullptr; }

    // True if neither GUID nor path has been assigned.
    bool isEmpty() const { return m_assetId.empty() && m_path.empty(); }

    // -- ID and path access (for serialization and inspector) ----------------

    const SString& getAssetId() const { return m_assetId; }
    void setAssetId(const SString& id) { m_assetId = id; }

    const SString& getPath() const { return m_path; }
    void setPath(const SString& path) { m_path = path; }

    // -- comparison ----------------------------------------------------------

    bool operator==(const TAssetRef& other) const
    {
        // GUID takes priority when both sides have one.
        if (!m_assetId.empty() && !other.m_assetId.empty())
            return m_assetId == other.m_assetId;
        // Fall back to path comparison.
        return m_path == other.m_path;
    }

    bool operator!=(const TAssetRef& other) const { return !(*this == other); }

private:
    // Mutable because resolve() may backfill the GUID from a path-based
    // lookup. This is a cache, not observable state.
    mutable SString m_assetId;
    SString m_path;
};

#endif // ASSET_REF_HANDLE_H