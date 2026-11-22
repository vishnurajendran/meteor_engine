//
// ui_widget_instance.h
//
// Engine-side state of one live widget. Owned by MViewportSubsystem; scripts
// only ever hold MUIWidget handles that resolve to it.
//

#ifndef UI_WIDGET_INSTANCE_H
#define UI_WIDGET_INSTANCE_H

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/ui/assets/ui_widget_asset.h"
#include "core/utils/sstring.h"
#include "ui_widget.h"

class IScriptInstance;
class MEngineDomain;
class MViewport;

namespace Rml
{
    class Element;
    class ElementDocument;
    class StyleSheetContainer;
}

struct SUIWidgetInstance
{
    // -- Identity ------------------------------------------------------------------
    uint32_t id         = 0;   // slot in the subsystem's table, from 1
    uint32_t generation = 0;   // handles must match
    uint32_t viewportId = 0;
    TAssetHandle<MUIWidgetAsset> asset;
    SString  name;             // the .muia's name: messages and the w-<Name> class
    MEngineDomain* domain = nullptr;   // destroyed with it

    // -- Structure -----------------------------------------------------------------
    // Top-level: `document` is the RmlUi document and `root` the same element.
    // Nested: `root` is the <widget> element in the top-level's document.
    Rml::ElementDocument* document = nullptr;
    Rml::Element*         root     = nullptr;
    uint32_t              parentId = 0;          // 0 = top-level
    std::vector<uint32_t> children;              // nested widgets, in document order

    // Moves whenever elements may have been replaced (a rebuild, setInnerRml),
    // so cached element lookups (MUIElement) know to search again.
    uint32_t domVersion = 1;

    // Top-level only: the document's own stylesheet (its <link>/<style>), with
    // the engine's base rules in front - widget styles are combined on top of it.
    std::shared_ptr<Rml::StyleSheetContainer> ownStyleSheet;
    // Top-level only: every .rcss asset path combined into the document, for
    // hot reload.
    std::vector<SString> styleSheetPaths;

    // -- Behaviour -----------------------------------------------------------------
    IScriptInstance* behaviour = nullptr;
    bool hasOnCreate         = false;
    bool hasOnAddToViewport  = false;
    bool hasOnRemove         = false;
    bool hasOnWidgetTick     = false;
    std::unordered_set<SString> reportedMissingMethods;   // "unknown method" logged once each

    // -- State ---------------------------------------------------------------------
    bool     inViewport     = false;   // top-level only - nested widgets follow theirs
    bool     destroyPending = false;   // destroyed from inside a script call; handles are already invalid
    int      zOrder         = 0;
    uint64_t addSequence    = 0;       // ties in zOrder: added later draws on top
    bool     visible        = true;
    bool     tickWhenPaused = true;

    [[nodiscard]] bool isTopLevel() const { return parentId == 0; }
    [[nodiscard]] MUIWidget handle() const { return MUIWidget(viewportId, id, generation); }
};

#endif // UI_WIDGET_INSTANCE_H
