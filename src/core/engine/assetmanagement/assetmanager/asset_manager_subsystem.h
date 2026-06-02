//
// Created by ssj5v on 18-05-2026.
//

#ifndef ASSET_MANAGER_SUBSYSTEM_H
#define ASSET_MANAGER_SUBSYSTEM_H
#include <map>


#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/subsystem/subsystem_interface.h"

class IAssetManagerSubsystem : public IEngineSubSystem {
public:
    IAssetManagerSubsystem() { instanceRef = this; }
    ~IAssetManagerSubsystem() override = default;
    virtual void refresh() = 0;

public:
    // Every handle returned resolves by GUID. The path is only used to
    // locate the asset and extract its GUID at creation time.
    template <typename T>
    TAssetHandle<T> getAsset(SString path) {
        static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

        if (assetMap.contains(path)) {
            auto* raw = dynamic_cast<T*>(assetMap[path]);
            if (raw && !raw->getAssetId().empty())
                return TAssetHandle<T>(raw->getAssetId(), &resolveByGuid);
        }
        return TAssetHandle<T>();
    }

    template <typename T>
    TAssetHandle<T> getAssetById(const SString& assetId) {
        static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

        if (assetMapByAssetId.contains(assetId))
            return TAssetHandle<T>(assetId, &resolveByGuid);
        return TAssetHandle<T>();
    }

    // Parses the "guid::<id>" / "rawp::<path>" serialisation format.
    //
    //   "guid::abc-123"           : getAssetById<T>("abc-123")
    //   "rawp::assets/tex.png"    : getAsset<T>("assets/tex.png")
    //
    // Both return a GUID handle. On re-save, toRefString() writes "guid::".
    // This lets scene files use human-readable paths as defaults that are
    // automatically upgraded to stable GUIDs once the asset is loaded.

    template <typename T>
    TAssetHandle<T> getAssetFromReference(const SString& refString) {
        static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

        const std::string& s = refString.str();

        if (s.size() > REF_PREFIX_LEN)
        {
            if (s.compare(0, REF_PREFIX_LEN, REF_GUID_PREFIX) == 0)
                return getAssetById<T>(SString(s.substr(REF_PREFIX_LEN)));

            if (s.compare(0, REF_PREFIX_LEN, REF_PATH_PREFIX) == 0)
                return getAsset<T>(SString(s.substr(REF_PREFIX_LEN)));
        }
        return TAssetHandle<T>();
    }

protected:
    // Named to discourage casual use. Exist for engine-internal hot paths
    // that need to avoid the handle overhead.
    //
    // WARNING: raw pointers become dangling after refresh(). Do not store them
    // across frames unless you know the asset will not be reloaded.

    template <typename T>
    T* internal_unsafe_getAsset(SString path) {
        static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

        if (assetMap.contains(path))
            return dynamic_cast<T*>(assetMap[path]);
        return nullptr;
    }

    template <typename T>
    T* internal_unsafe_getAssetById(const SString& assetId) {
        static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

        if (assetMapByAssetId.contains(assetId))
            return dynamic_cast<T*>(assetMapByAssetId[assetId]);
        return nullptr;
    }

    // Single non-template static function returning MAsset*. Shared by every
    // TAssetHandle regardless of T. Type narrowing happens in
    // TAssetHandle::get() via dynamic_cast<T*>.

    static MAsset* resolveByGuid(const SString& assetId)
    {
        auto* mgr = instanceRef;
        if (mgr->assetMapByAssetId.contains(assetId))
            return mgr->assetMapByAssetId[assetId];
        return nullptr;
    }

protected:
    const SString META_FILE_EXTENSION = "meta";
    const SString ASSET_FILE_TAG = "asset_id";
    const SString ASSET_ID_ATTRIB = "id";

    // Used by getAssetFromReference() when loading saved data.
    // "rawp::" references are converted to GUID handles on load and
    // re-saved as "guid::" - the path form is a load-time convenience only.
    static constexpr const char* REF_GUID_PREFIX = "guid::";
    static constexpr const char* REF_PATH_PREFIX = "rawp::";
    static constexpr size_t      REF_PREFIX_LEN  = 6;

    std::map<SString, MAsset*> assetMap;
    std::map<SString, MAsset*> assetMapByAssetId;
    static IAssetManagerSubsystem* instanceRef;
};

#endif //ASSET_MANAGER_SUBSYSTEM_H
