//
// ui_asset_creators.cpp
//

#include "ui_asset_creators.h"

#include "builtin_asset_templates.h"
#include "core/engine/scripting/lua/lua_script_asset.h"
#include "core/engine/ui/assets/ui_structure_asset.h"
#include "core/engine/ui/assets/ui_style_asset.h"
#include "core/engine/ui/assets/ui_widget_asset.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "editorassetmanager.h"

namespace
{
    // Templates use __NAME__ (the widget's name, also its CSS class suffix)
    // and __CLASS__ (the behaviour's Lua class name).
    SString fill(const char* text, const SString& name, const SString& className = SString())
    {
        SString out(text);
        out.replace("__NAME__", name);
        if (!className.empty())
            out.replace("__CLASS__", className);
        return out;
    }

    SString identifier(const SString& name, const char* fallback)
    {
        const SString id = MAssetTemplateRegistry::toIdentifier(name.trimmed());
        return id.empty() ? SString(fallback) : id;
    }

    // "assets/ui/Hud_1.rml" -> "Hud_1"
    SString stemOf(const SString& path)
    {
        return FileIO::getFileNameWithoutExtension(path);
    }

    constexpr const char* STRUCTURE_TEMPLATE = R"RML(<rml>
<head>
    <title>__NAME__</title>
    <!--
        The widget's styles come from its .muia (Style), so nothing needs
        linking here. A <link type="text/rcss" href="..."/> still works.
    -->
</head>
<body>
    <!--
        __NAME__. Ids only need to be unique within this widget: its behaviour
        finds them with self.widget:findElement("id").

        on<event> attributes name a method on the behaviour (onclick, onchange,
        onmouseover, ...). Nest another widget with
            <widget src="guid:..." id="name"/>
        (drag its .muia's GUID in from the inspector).
    -->
    <div class="panel">
        <h1 id="title">__NAME__</h1>
        <p id="message">Hello from __NAME__.</p>
        <button id="button" onclick="onButtonClicked">Click me</button>
    </div>
</body>
</rml>
)RML";

    constexpr const char* STYLE_TEMPLATE = R"RCSS(/*
    __NAME__ styles.

    Every rule is scoped under .w-__NAME__, the class the widget's root gets.
    CSS isn't scoped per widget: when this widget is nested in another, both
    share one stylesheet, and an unscoped rule would reach the other widget's
    elements too. Keep new rules under .w-__NAME__.

    "Meteor Sans" is the engine's default font (Open Sans). Any font asset in
    the project can be used by its family name.
*/

.w-__NAME__
{
    font-family: "Meteor Sans";
    font-size: 18dp;
    color: #ffffff;
}

.w-__NAME__ .panel
{
    display: block;
    position: absolute;
    left: 40dp;
    top: 40dp;
    width: 360dp;
    padding: 16dp;
    background-color: #1e1e28d0;
    border-radius: 6dp;
}

.w-__NAME__ h1
{
    display: block;
    font-size: 26dp;
    font-weight: bold;
    margin-bottom: 8dp;
}

.w-__NAME__ p
{
    display: block;
    margin-bottom: 14dp;
}

.w-__NAME__ button
{
    display: inline-block;
    padding: 6dp 16dp;
    background-color: #3c64c8;
    border-radius: 4dp;
    cursor: pointer;
}

.w-__NAME__ button:hover
{
    background-color: #5078dc;
}

.w-__NAME__ button:active
{
    background-color: #2d4ea0;
}
)RCSS";

    constexpr const char* WIDGET_BEHAVIOUR_TEMPLATE = R"LUA(--- Optional: put this script's class in a module (namespace). Without it the
--- class is imported as "meteor.game.__CLASS__".
--- MODULE "game.ui"

--- Behaviour of the __NAME__ widget. Gameplay creates the widget and calls
--- methods on this behaviour:
---
---   local ui = MUIWidget.create("guid:...")   -- __NAME__.muia (drag it in to get its GUID)
---   ui:addToViewport(0)
---   ui:getBehaviour():showMessage("Hi")
local __CLASS__ = CLASS("__CLASS__", WidgetBehaviour) {
    --- Fields. Look elements up in onCreate: it runs again when the .rml
    --- is hot-reloaded. Start a name with _ to keep it private.
    _message = nil,



    --- The widget's elements exist; it is not on screen yet. Nested widgets'
    --- behaviours have already run their onCreate.
    --- @param self __CLASS__
    onCreate = function(self)
        self._message = self.widget:findElement("message")
    end,



    --- Each time the widget is added to the viewport.
    --- @param self __CLASS__
    onAddToViewport = function(self)

    end,



    --- The widget left the viewport: removed, destroyed, or Play stopped.
    --- @param self __CLASS__
    onRemove = function(self)

    end,



    --- Every frame while in the viewport, after every entity's onLateTick.
    --- While the game is paused only if "Tick when paused" is on.
    --- @param self __CLASS__
    --- @param dt number real time since the last frame
    onWidgetTick = function(self, dt)

    end,



    --- Called by gameplay through getBehaviour().
    --- @param self __CLASS__
    --- @param text string
    showMessage = function(self, text)
        self._message:setText(text)
    end,



    --- From onclick="onButtonClicked" in __NAME__.rml.
    --- @param self __CLASS__
    --- @param event SUIEvent
    onButtonClicked = function(self, event)
        self._message:setText("Clicked!")
    end,
}

return __CLASS__
)LUA";

    // -- .rml ---------------------------------------------------------------------------

    class MUIStructureCreator : public TAssetCreator<MUIStructureAsset, SUIStructureInitData>
    {
    public:
        SString getExtension()   const override { return ".rml"; }
        SString getDisplayName() const override { return "UI Structure"; }
        SString getMenuPath()    const override { return "UI/Structure (.rml)"; }
        SString getDefaultName() const override { return "New_Structure"; }

    protected:
        SString createTyped(MEditorAssetManager& manager, const SString& directory, const SString& name,
                            const SUIStructureInitData& data) override
        {
            const SString path   = manager.makeUniqueAssetPath(directory, identifier(name, "Structure"), getExtension());
            const SString widget = data.widgetName.empty() ? stemOf(path) : data.widgetName;
            return manager.writeAndImport(path, fill(STRUCTURE_TEMPLATE, widget)) ? path : SString();
        }
    };

    // -- .rcss --------------------------------------------------------------------------

    class MUIStyleCreator : public TAssetCreator<MUIStyleAsset, SUIStyleInitData>
    {
    public:
        SString getExtension()   const override { return ".rcss"; }
        SString getDisplayName() const override { return "UI Style Sheet"; }
        SString getMenuPath()    const override { return "UI/Style Sheet (.rcss)"; }
        SString getDefaultName() const override { return "New_Style"; }

    protected:
        SString createTyped(MEditorAssetManager& manager, const SString& directory, const SString& name,
                            const SUIStyleInitData& data) override
        {
            const SString path   = manager.makeUniqueAssetPath(directory, identifier(name, "Style"), getExtension());
            const SString widget = data.widgetName.empty() ? stemOf(path) : data.widgetName;
            return manager.writeAndImport(path, fill(STYLE_TEMPLATE, widget)) ? path : SString();
        }
    };

    // -- .lua ---------------------------------------------------------------------------

    class MLuaScriptCreator : public TAssetCreator<MLuaScriptAsset, SLuaScriptInitData>
    {
    public:
        SString getExtension()   const override { return ".lua"; }
        SString getDisplayName() const override { return "Lua Script"; }
        // Not in the menu: "Script" there is the text template.
        SString getDefaultName() const override { return "NewScript"; }

    protected:
        SString createTyped(MEditorAssetManager& manager, const SString& directory, const SString& name,
                            const SLuaScriptInitData& data) override
        {
            const SString path      = manager.makeUniqueAssetPath(directory, identifier(name, "Script"), getExtension());
            const SString className = MAssetTemplateRegistry::toIdentifier(stemOf(path));

            SString content;
            if (data.baseClass == "WidgetBehaviour")
            {
                content = fill(WIDGET_BEHAVIOUR_TEMPLATE, data.widgetName.empty() ? className : data.widgetName, className);
            }
            else if (!manager.getTemplateRegistry().generate(SBuiltInTemplateIds::LuaScript, directory, stemOf(path), {}, content))
            {
                return {};
            }
            return manager.writeAndImport(path, content) ? path : SString();
        }
    };

    // -- UI widget (folder) --------------------------------------------------------------

    class MUIWidgetCreator : public TAssetCreator<MUIWidgetAsset>
    {
    public:
        SString getExtension()   const override { return ".muia"; }
        SString getDisplayName() const override { return "UI Widget"; }
        SString getMenuPath()    const override { return "UI Widget"; }
        SString getDefaultName() const override { return "New_Widget"; }

    protected:
        SString createTyped(MEditorAssetManager& manager, const SString& directory, const SString& name,
                            const SAssetInitialisationData&) override
        {
            const SString widget = identifier(name, "Widget");

            // UI_<Name>, or UI_<Name>_1 ... if taken.
            const SString folder     = manager.makeUniqueAssetPath(directory, SString("UI_") + widget, "");
            const SString folderName = FileIO::getFileName(folder);
            if (!manager.createDirectory(directory, folderName) || !manager.createDirectory(folder, "res"))
            {
                MERROR("Create UI Widget: could not create the folder " + folder);
                return {};
            }
            const SString res = folder + "/res";

            auto failed = [&](const char* part)
            {
                MERROR(SString::format("Create UI Widget: could not create the {0} in {1} - no .muia was written; "
                                       "the folder is left as it is", SString(part), res));
                return SString();
            };

            SUIStructureInitData structureData;
            structureData.widgetName = widget;
            const auto structure = manager.createAsset<MUIStructureAsset>(res, widget, structureData);
            if (!structure)
                return failed("structure (.rml)");

            SUIStyleInitData styleData;
            styleData.widgetName = widget;
            const auto style = manager.createAsset<MUIStyleAsset>(res, widget, styleData);
            if (!style)
                return failed("style (.rcss)");

            SLuaScriptInitData scriptData;
            scriptData.baseClass  = "WidgetBehaviour";
            scriptData.widgetName = widget;
            const auto behaviour = manager.createAsset<MLuaScriptAsset>(res, widget, scriptData);
            if (!behaviour)
                return failed("behaviour (.lua)");

            // The parts by GUID, so moving or renaming them keeps the widget intact.
            const SString path = folder + "/" + widget + getExtension();
            const SString text = MUIWidgetAsset::serialise(structure.toRefString(), { style.toRefString() },
                                                           behaviour.toRefString(), true);
            return manager.writeAndImport(path, text) ? path : SString();
        }
    };
}

REGISTER_ASSET_CREATOR(MUIWidgetAsset,    MUIWidgetCreator)
REGISTER_ASSET_CREATOR(MUIStructureAsset, MUIStructureCreator)
REGISTER_ASSET_CREATOR(MUIStyleAsset,     MUIStyleCreator)
REGISTER_ASSET_CREATOR(MLuaScriptAsset,   MLuaScriptCreator)
