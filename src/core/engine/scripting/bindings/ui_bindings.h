//
// ui_bindings.h
//
// Hand-written parts of the UI script API, registered after the generated
// bindings:
//
//   MUIWidget.create(reference)   also takes a path or "guid:..." string
//                                 (the generated one takes an MAssetHandle)
//   widget:getBehaviour()         the widget's behaviour table, or nil - the
//                                 generator has no Lua-table type
//
//   local hud = MUIWidget.create("assets/UI_Hud/Hud.muia")
//   hud:getBehaviour():setHealth(100)
//

#pragma once
#ifndef METEOR_UI_BINDINGS_H
#define METEOR_UI_BINDINGS_H

#include <sol/sol.hpp>

#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/scripting/lua/lua_script_instance.h"
#include "core/engine/ui/widget/ui_widget.h"
#include "core/engine/ui/widget/ui_widget_instance.h"
#include "core/utils/sstring_sol_traits.h"

namespace MeteorBindings {

static void registerUIBindings(sol::state& lua)
{
    sol::object widgetType = lua["MUIWidget"];
    if (widgetType.get_type() != sol::type::table && widgetType.get_type() != sol::type::userdata)
        return;   // the generated bindings didn't register MUIWidget

    lua["MUIWidget"]["create"] = sol::overload(
        [](const SString& reference) { return MUIWidget::createFromReference(reference); },
        [](const TAssetHandle<MAsset>& asset) { return MUIWidget::create(strictAssetCast<MUIWidgetAsset>(asset)); });

    lua["MUIWidget"]["getBehaviour"] = [](const MUIWidget& self, sol::this_state ts) -> sol::object
    {
        const SUIWidgetInstance* widget = self.resolve();
        auto* behaviour = widget ? dynamic_cast<MLuaScriptInstance*>(widget->behaviour) : nullptr;
        if (!behaviour || !behaviour->isValid())
            return sol::make_object(ts, sol::lua_nil);

        // Only within the Lua state that created it (the same domain).
        sol::table& table = behaviour->getInstanceTable();
        if (sol::main_thread(ts) != sol::main_thread(table.lua_state()))
            return sol::make_object(ts, sol::lua_nil);
        return sol::make_object(ts, table);
    };
}

} // namespace MeteorBindings

#endif // METEOR_UI_BINDINGS_H
