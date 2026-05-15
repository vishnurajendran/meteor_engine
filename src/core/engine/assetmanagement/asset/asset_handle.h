// asset_handle.h
//
// A stable, typed reference to an asset. Stores the asset's GUID and a
// resolve callback. The handle always resolves by GUID — it does not know
// about file paths or the asset manager.
//
// ── Usage ─────────────────────────────────────────────────────────────────────
//   TAssetHandle<MTextureAsset> tex = manager->getAsset<MTextureAsset>(path);
//   tex->getTexture()        — operator-> resolves via callback + dynamic_cast
//   tex.get()                — explicit resolve, may return nullptr
//   if (tex) { ... }         — bool cast, false when asset not found
//
// ── Converting between types ──────────────────────────────────────────────────
//   TAssetHandle<MAsset>            base = manager->getAsset<MAsset>(path);
//   TAssetHandle<MStaticMeshAsset>  mesh = base;   // downcast — safe at runtime
//   TAssetHandle<MAsset>            back = mesh;    // upcast  — always valid
//
//   Downcasts that don't match return nullptr from get().
//
// ── Serialisation ─────────────────────────────────────────────────────────────
//   toRefString()  produces "guid::<id>".
//   The manager also accepts "rawp::<path>" at load time, but that is
//   converted to a GUID handle immediately. On re-save the reference is
//   always written as "guid::<id>".
//
// ── Creation ──────────────────────────────────────────────────────────────────
//   Handles should only be created through MAssetManager.
//   Do not construct handles manually unless you supply a valid resolver.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once
#ifndef ASSETHANDLE_H
#define ASSETHANDLE_H

#include <type_traits>
#include "core/engine/assetmanagement/asset/asset.h"
// NOTE: No include of assetmanager.h — the handle is manager-agnostic.

template<typename T>
class TAssetHandle
{
    static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

    // All specialisations are friends so converting constructors can
    // read private members of TAssetHandle<U>.
    template<typename U> friend class TAssetHandle;

public:
    // ── Resolver type ─────────────────────────────────────────────────────────
    // Returns MAsset* (untyped) so the same function pointer works across all
    // handle types. get() applies dynamic_cast<T*> for type safety.
    using ResolveFunc = MAsset*(*)(const SString&);

private:
    SString     assetId;                          // always a GUID
    ResolveFunc resolveFunc = nullptr;

public:
    TAssetHandle() = default;

    // Primary constructor — called by the asset manager when handing out handles.
    TAssetHandle(const SString& id, ResolveFunc resolver)
        : assetId(id), resolveFunc(resolver) {}

    // ── Converting constructor ────────────────────────────────────────────────
    // Allows conversion between any TAssetHandle<U> and TAssetHandle<T> as
    // long as both U and T derive from MAsset.
    //
    // Upcast   (Derived -> Base):  always valid.
    // Downcast (Base -> Derived):  compiles, but get() returns nullptr if the
    //                              underlying asset isn't actually a T.
    //
    // The resolver is copied as-is because it returns MAsset* — the
    // dynamic_cast<T*> in get() handles type narrowing.
    template<typename U>
    TAssetHandle(const TAssetHandle<U>& other)  // NOLINT(google-explicit-constructor)
        : assetId(other.assetId)
        , resolveFunc(other.resolveFunc)
    {
        static_assert(std::is_base_of_v<MAsset, U>, "U must inherit from MAsset");
    }

    // ── Resolution ────────────────────────────────────────────────────────────

    // Resolves through the callback every call, then dynamic_cast<T*> for
    // type safety. Returns nullptr if the handle is null, the asset isn't
    // loaded, or the asset isn't actually a T.
    T* get() const
    {
        if (assetId.empty() || !resolveFunc)
            return nullptr;
        return dynamic_cast<T*>(resolveFunc(assetId));
    }

    T* operator->() const { return get(); }
    T& operator*()  const { return *get(); }

    // ── State queries ─────────────────────────────────────────────────────────

    // True if the handle has a GUID AND the asset is currently loaded
    // AND the asset is actually a T.
    explicit operator bool() const { return isValid(); }
    bool isValid() const { return get() != nullptr; }

    // True if the handle was never assigned (no GUID at all).
    bool isNull() const { return assetId.empty(); }

    // ── Accessors ─────────────────────────────────────────────────────────────

    const SString& getAssetId() const { return assetId; }

    // ── Serialisation ─────────────────────────────────────────────────────────

    // Always produces "guid::<id>". Returns empty string for null handles.
    SString toRefString() const
    {
        if (assetId.empty()) return SString();
        return SString("guid::") + assetId;
    }

    // ── Comparison ────────────────────────────────────────────────────────────

    bool operator==(const TAssetHandle<T>& other) const { return assetId == other.assetId; }
    bool operator!=(const TAssetHandle<T>& other) const { return assetId != other.assetId; }
};

#endif // ASSETHANDLE_H