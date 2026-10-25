//
// Created by ssj5v on 18-05-2026.
//

#ifndef ASSET_MANAGER_SUBSYSTEM_H
#define ASSET_MANAGER_SUBSYSTEM_H
#include <map>
#include <memory>
#include <vector>

#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/subsystem/subsystem_interface.h"

class IAssetSource;

class IAssetManagerSubsystem : public IEngineSubSystem {
public:
    IAssetManagerSubsystem() { instanceRef = this; }
    ~IAssetManagerSubsystem() override = default;
    virtual void refresh() = 0;

    // Where asset bytes come from: the project folder(s) in the editor, the
    // package in a published game. Safe to call from any thread - the
    // shared_ptr keeps the source alive even if the main thread swaps it
    // mid-call. Never nullptr. Most code goes through MAssetSources::getActive().
    [[nodiscard]] virtual std::shared_ptr<IAssetSource> getActiveAssetSource() = 0;

public:
    // Every handle returned resolves by GUID. The path is only used to
    // locate the asset and extract its GUID at creation time.
    template <typename T>
    TAssetHandle<T> getAsset(SString path) {
        static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

        if (!assetMap.contains(path))
            loadOnDemand(path);

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

        if (!assetMapByAssetId.contains(assetId))
            loadOnDemandById(assetId);

        if (assetMapByAssetId.contains(assetId))
            return TAssetHandle<T>(assetId, &resolveByGuid);
        return TAssetHandle<T>();
    }

    // Parses a serialized asset reference.
    //
    //   "guid:abc-123"            : getAssetById<T>("abc-123")
    //   "assets/tex.png"          : getAsset<T>("assets/tex.png")   — no prefix = path
    //   ""                        : null handle
    //
    // Both return a GUID handle. On re-save, toRefString() writes "guid:".
    // This lets hand-written files use human-readable paths that are
    // automatically upgraded to stable GUIDs the next time they are saved.
    //
    // Cost: one prefix compare + one std::map lookup — same order as getAsset().

    template <typename T>
    TAssetHandle<T> getAssetFromReference(const SString& refString) {
        static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

        const std::string& s = refString.str();
        if (s.empty())
            return TAssetHandle<T>();

        if (isGuidReference(refString))
            return getAssetById<T>(SString(s.substr(ASSET_REF_GUID_PREFIX_LEN)));

        return getAsset<T>(refString);
    }

    // -- Core assets -------------------------------------------------------------
    // Engine files looked up by name instead of GUID, with no meta and no
    // MAsset: shader include files and the Lua engine modules. Names are
    // "<prefix><file>", e.g. "shader_utils/meteor_vertex_utils.glsl".
    //
    // Only a published game provides them (from the package). Otherwise
    // providesCoreAssets() is false and the systems that need these files
    // read them from the engine install instead.
    static constexpr const char* CORE_SHADER_UTILS = "shader_utils/";   // shader #include files
    static constexpr const char* CORE_LUA_MODULES  = "lua_modules/";    // Behaviour, MMath, ...

    [[nodiscard]] virtual bool providesCoreAssets() const { return false; }
    // Every core asset name starting with `prefix`, sorted.
    [[nodiscard]] virtual std::vector<SString> listCoreAssets(const SString& /*prefix*/) const { return {}; }
    // False if there is no core asset with that name.
    virtual bool loadCoreAsset(const SString& /*name*/, SString& /*outData*/) const { return false; }

    // True if the string uses the "guid:<id>" form.
    static bool isGuidReference(const SString& refString)
    {
        const std::string& s = refString.str();
        return s.size() > ASSET_REF_GUID_PREFIX_LEN
            && s.compare(0, ASSET_REF_GUID_PREFIX_LEN, ASSET_REF_GUID_PREFIX) == 0;
    }

    // Rewrites any reference (path or guid) into the canonical "guid:<id>" form.
    // If the asset can't be found the original string is returned unchanged, so
    // a broken reference is never silently erased on save.
    template <typename T = MAsset>
    SString toCanonicalReference(const SString& refString)
    {
        const auto handle = getAssetFromReference<T>(refString);
        return handle.isValid() ? handle.toRefString() : refString;
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

        if (!assetMap.contains(path))
            loadOnDemand(path);

        if (assetMap.contains(path))
            return dynamic_cast<T*>(assetMap[path]);
        return nullptr;
    }

    template <typename T>
    T* internal_unsafe_getAssetById(const SString& assetId) {
        static_assert(std::is_base_of_v<MAsset, T>, "T must inherit from MAsset");

        if (!assetMapByAssetId.contains(assetId))
            loadOnDemandById(assetId);

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
        return mgr->loadOnDemandById(assetId);
    }

    // Lazy-loading hooks, called when a lookup misses. The editor imports
    // every asset up front and keeps these as no-ops; the player imports the
    // asset from the package on first use (see MPlayerAssetManager). Return
    // the asset, or nullptr if it does not exist.
    virtual MAsset* loadOnDemand(const SString& /*path*/)        { return nullptr; }
    virtual MAsset* loadOnDemandById(const SString& /*assetId*/) { return nullptr; }

protected:
    const SString META_FILE_EXTENSION = "meta";
    const SString ASSET_FILE_TAG = "asset_id";
    const SString ASSET_ID_ATTRIB = "id";

    std::map<SString, MAsset*> assetMap;
    std::map<SString, MAsset*> assetMapByAssetId;
    static IAssetManagerSubsystem* instanceRef;
};

#endif //ASSET_MANAGER_SUBSYSTEM_H
