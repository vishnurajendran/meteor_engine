//
// asset_bindings.h
//
// Manual Sol2 binding for asset handles. TAssetHandle<T> is a template, so
// the annotation generator can't bind it; scripts get ONE handle type,
// MAssetHandle (= TAssetHandle<MAsset>). The generator converts at the
// boundary: a bound function returning TAssetHandle<T> / TAssetRef<T> hands
// Lua an MAssetHandle, and one taking them accepts an MAssetHandle and
// narrows it. Both use strictAssetCast, so a handle to the wrong asset type
// becomes a null handle.
//
// A handle stores the asset's GUID and looks the asset up on every get(),
// so it stays safe across asset reloads. get() returns the asset itself -
// use it right away, don't keep it. Cast it to the concrete type:
//
//   local handle = MAssetManager.find("assets/ui/logo.png")
//   local texture = MTextureAsset(handle:get())   -- nil if not a texture
//
// Call MeteorBindings::registerAssetBindings(lua) with the other bindings.
//

#pragma once
#ifndef METEOR_ASSET_BINDINGS_H
#define METEOR_ASSET_BINDINGS_H

#include <string>

#include <sol/sol.hpp>
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/sstring_sol_traits.h"

// TAssetHandle has operator-> and operator*, so sol2 treats it as pointer-like
// and its automatic operators (==, tostring, ...) dereference it to MAsset,
// which has none - a compile error. Turn them off; the binding below defines
// == (same GUID) and tostring itself.
namespace sol {
    template <>
    struct is_automagical<TAssetHandle<MAsset>> : std::false_type {};
}

namespace MeteorBindings {

static void registerAssetBindings(sol::state& lua) {
    using Handle = TAssetHandle<MAsset>;

    lua.new_usertype<Handle>("MAssetHandle",
        sol::no_constructor,

        // The asset now, or nil if the handle is null or the asset isn't
        // loaded. Re-fetch instead of storing the result.
        "get",         [](const Handle& h) -> MAsset* { return h.get(); },
        "isValid",     [](const Handle& h) { return h.isValid(); },
        "isNull",      [](const Handle& h) { return h.isNull(); },
        "getAssetId",  [](const Handle& h) { return h.getAssetId(); },
        // Empty when the asset isn't loaded.
        "getPath",     [](const Handle& h) -> SString {
            const MAsset* asset = h.get();
            return asset ? asset->getPath() : SString();
        },
        // "guid:<id>", the form scene files store.
        "toRefString", [](const Handle& h) { return h.toRefString(); },

        // Handle for an asset object you already have (e.g. from a getter).
        "fromAsset",   [](MAsset* asset) -> Handle {
            auto* manager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
            if (!asset || !manager) return Handle();
            return manager->getAssetById<MAsset>(asset->getAssetId());
        },
        // A null handle - pass it to clear an asset slot.
        "none",        []() { return Handle(); },

        sol::meta_function::equal_to, [](const Handle& a, const Handle& b) { return a == b; },
        sol::meta_function::to_string, [](const Handle& h) {
            if (h.isNull()) return std::string("MAssetHandle(null)");
            const MAsset* asset = h.get();
            return "MAssetHandle(" + (asset ? asset->getPath().str() : h.toRefString().str() + ", not loaded") + ")";
        }
    );
}

} // namespace MeteorBindings

#endif // METEOR_ASSET_BINDINGS_H
