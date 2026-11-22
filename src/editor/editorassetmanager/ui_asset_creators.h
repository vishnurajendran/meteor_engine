//
// ui_asset_creators.h
//
// Asset creators for game UI (asset_creator.h):
//
//   Create > UI Widget                a folder with everything a widget needs:
//                                       UI_<Name>/<Name>.muia
//                                       UI_<Name>/res/<Name>.rml   sample element + onclick
//                                       UI_<Name>/res/<Name>.rcss  rules scoped under .w-<Name>
//                                       UI_<Name>/res/<Name>.lua   WidgetBehaviour, every hook stubbed
//   Create > UI > Structure (.rml)
//   Create > UI > Style Sheet (.rcss)
//   createAsset<MLuaScriptAsset>       code only: a Behaviour or WidgetBehaviour script
//
// If a part of a widget fails, nothing more is written (no .muia) and the
// error names the part; the partly filled folder is left for the user.
//

#ifndef UI_ASSET_CREATORS_H
#define UI_ASSET_CREATORS_H

#include "asset_creator.h"

struct SUIStructureInitData : SAssetInitialisationData
{
    SString widgetName;   // for the sample content; empty = the file name
};

struct SUIStyleInitData : SAssetInitialisationData
{
    SString widgetName;   // rules are scoped under .w-<widgetName>; empty = the file name
};

struct SLuaScriptInitData : SAssetInitialisationData
{
    SString baseClass = "Behaviour";   // or "WidgetBehaviour"
    SString widgetName;                // WidgetBehaviour: the widget it belongs to, for comments
};

#endif // UI_ASSET_CREATORS_H
