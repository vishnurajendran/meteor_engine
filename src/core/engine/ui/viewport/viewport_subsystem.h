//
// viewport_subsystem.h
//
// MViewportSubsystem - owns screen-space game UI (RmlUi):
//
//   - RmlUi's lifetime and interfaces (system, file, GL3 renderer, events),
//     started on first use, once a GL context exists
//   - the viewports (v1 makes one: the player window, or the editor's Scene
//     view during Play) and their RmlUi contexts
//   - every live widget: building documents from .muia/.rml/.rcss, nested
//     <widget> elements, behaviours and their hooks, z-order, teardown when
//     the script domain that created them goes away (Stop)
//   - fonts: every font asset is registered with RmlUi; the engine's Open Sans
//     is registered as the family "Meteor Sans", the default and fallback face
//   - hot reload of live widgets when their .rml, .rcss, .muia, textures or
//     fonts change
//
// Like animation, canTick() stays false: the applications call update() at a
// fixed point in the frame (after lateUpdate) and MUIRenderStage draws it.
//
//   input (UI first) -> physics -> onTick -> animation -> onLateTick
//     -> update(): onWidgetTick, then Rml::Context::Update
//     -> render: scene stages -> post -> composite -> MUIRenderStage
//

#ifndef VIEWPORT_SUBSYSTEM_H
#define VIEWPORT_SUBSYSTEM_H

#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/engine/subsystem/subsystem_interface.h"
#include "core/engine/ui/widget/ui_widget.h"
#include "core/engine/ui/widget/ui_widget_instance.h"
#include "core/object/object.h"
#include "core/utils/delegate.h"
#include "viewport.h"

class MAsset;
class MEngineDomain;
class MFontAsset;
class MRmlDocumentPlugin;
class MRmlEventListenerInstancer;
class MRmlFileInterface;
class MRmlRenderInterface;
class MRmlSystemInterface;
class MUIStructureAsset;

namespace Rml
{
    class Element;
    class ElementDocument;
    class Event;
    class StyleSheetContainer;
}

class MViewportSubsystem : public MObject, public IEngineSubSystem
{
    DEFINE_OBJECT_SUBCLASS(MViewportSubsystem)
public:
    // The family documents use for the engine's default font.
    static constexpr const char* DEFAULT_FONT_FAMILY = "Meteor Sans";
    // Engine font files registered under DEFAULT_FONT_FAMILY.
    static constexpr const char* DEFAULT_FONT_FOLDER = "meteor_assets/engine_assets/fonts/open-sans/";
    static constexpr const char* DEFAULT_FONT_PREFIX = "OpenSans-";

    MViewportSubsystem();
    ~MViewportSubsystem() override;

    // True for the engine font files registered as DEFAULT_FONT_FAMILY.
    [[nodiscard]] static bool isDefaultFontPath(const SString& path);

    void init() override;
    void cleanup() override;

    // -- Viewports ---------------------------------------------------------------------
    // The first viewport created is the main one, which the script API targets.
    MViewport* createViewport(IViewportHost* host);
    void       destroyViewport(uint32_t viewportId);
    [[nodiscard]] MViewport* getMainViewport() const;
    [[nodiscard]] MViewport* getViewport(uint32_t viewportId) const;

    // -- Frame -------------------------------------------------------------------------
    // Widget ticks (onWidgetTick), then every context's Update. Call once per
    // frame after the scene's lateUpdate. dt is the real frame time, paused or not.
    void update(float deltaTime);

    // Draws a viewport into `framebuffer` (w x h pixels). Called by MUIRenderStage.
    void renderViewport(MViewport& viewport, unsigned int framebuffer, int width, int height);

    // -- Widgets -----------------------------------------------------------------------
    MUIWidget createWidget(const TAssetHandle<MUIWidgetAsset>& asset);
    [[nodiscard]] SUIWidgetInstance* resolve(const MUIWidget& handle) const;
    [[nodiscard]] SUIWidgetInstance* getInstance(uint32_t widgetId) const;

    void addToViewport(const MUIWidget& handle, int zOrder);
    void removeFromViewport(const MUIWidget& handle);
    void destroyWidget(const MUIWidget& handle);
    void setWidgetVisible(const MUIWidget& handle, bool visible);

    // This widget's element with that id, not searching nested widgets.
    [[nodiscard]] Rml::Element* findElement(const SUIWidgetInstance& widget, const SString& elementId) const;
    // A nested widget directly inside `widget` with that <widget id>.
    [[nodiscard]] SUIWidgetInstance* findChildWidget(const SUIWidgetInstance& widget, const SString& widgetId) const;
    // The top-level widget `widget` is (nested) in.
    [[nodiscard]] SUIWidgetInstance* getTopLevel(const SUIWidgetInstance& widget) const;

    // True when the widget (or the top-level widget it is nested in) is in
    // its viewport.
    [[nodiscard]] bool isShown(const SUIWidgetInstance& widget) const;

    // Replaces an element's content with RML (MUIElement setText /
    // setInnerRml): nested widgets inside it are destroyed first, any <widget>
    // in the new content becomes a nested widget, and cached element lookups
    // for the owner are invalidated.
    void replaceElementContent(const MUIWidget& owner, Rml::Element* element, const SString& rml);

    // From an on<event> attribute (MRmlEventListener).
    void dispatchElementEvent(Rml::Element* listenerElement, const SString& methodName, Rml::Event& event);
    [[nodiscard]] uint64_t getCurrentDispatchSerial() const { return currentDispatchSerial; }

    // -- Editor ------------------------------------------------------------------------
    // RmlUi's debugger over the main viewport (editor builds only; no-op otherwise).
    void setDebuggerVisible(bool visible);
    [[nodiscard]] bool isDebuggerVisible() const { return debuggerVisible; }

    [[nodiscard]] bool isRmlInitialised() const { return rmlInitialised; }

private:
    // -- RmlUi setup ---------------------------------------------------------------------
    // Starts RmlUi on first use. False (logged once) if it can't, e.g. the GL3
    // renderer failed to compile its shaders.
    bool ensureRml();
    void shutdownRml();

    // -- Fonts -----------------------------------------------------------------------------
    void registerAllFonts();
    void registerFont(MFontAsset* font);

    // -- Building ------------------------------------------------------------------------
    SUIWidgetInstance* allocateInstance();
    void freeInstance(uint32_t widgetId);

    // Loads the top-level widget's document and everything nested in it.
    bool buildDocument(SUIWidgetInstance& widget);
    // RmlUi parsed the document buildDocument is loading, and is about to lay
    // it out: give it its stylesheet now, so its text never formats unstyled
    // ("No font face defined").
    void onDocumentLoad(Rml::ElementDocument* document);
    // Base sheet + the document's own <link>/<style>.
    [[nodiscard]] std::shared_ptr<Rml::StyleSheetContainer> makeOwnStyleSheet(const Rml::ElementDocument* document) const;
    // Turns every <widget> element under `subtree` that belongs to `owner` into
    // a nested widget. `chain` holds the .muia GUIDs from the top down (cycles).
    void expandNestedWidgets(uint32_t ownerId, Rml::Element* subtree, std::vector<SString>& chain);
    // Fills a nested widget's element from its .rml body and expands it in turn.
    void buildNestedContent(uint32_t widgetId, std::vector<SString>& chain);
    // Combines the base sheet, the document's own and every widget's .rcss.
    void applyStyles(SUIWidgetInstance& topLevel);
    void collectStylePaths(const SUIWidgetInstance& widget, std::vector<SString>& out) const;

    // Behaviours for `widgetId` and everything nested in it that has none yet.
    void createBehaviours(uint32_t widgetId);
    void createBehaviour(SUIWidgetInstance& widget);
    void releaseBehaviour(SUIWidgetInstance& widget);

    // -- Lifecycle hooks (each resolves the widget again after calling Lua) --------------
    void callOnCreateTree(uint32_t widgetId);           // children first, then the widget
    void callOnAddToViewportTree(uint32_t widgetId);    // the widget, then its children
    void callOnRemoveTree(uint32_t widgetId);           // the widget, then its children
    void tickTree(uint32_t widgetId, float deltaTime, bool paused);
    bool callHook(uint32_t widgetId, const char* hook, bool (SUIWidgetInstance::*flag));
    bool callHook(uint32_t widgetId, const char* hook, bool (SUIWidgetInstance::*flag), float value);

    // Frees `widgetId`'s nested widgets (behaviours released, mappings
    // removed). Their elements are left to whoever replaces or closes them.
    void destroyChildren(uint32_t widgetId);
    void destroyTree(uint32_t widgetId);

    void restack(MViewport& viewport);

    // .muia GUIDs from the top-level widget down to `widget` (cycle detection).
    [[nodiscard]] std::vector<SString> chainFor(const SUIWidgetInstance& widget) const;
    [[nodiscard]] static bool isDescendantOf(const Rml::Element* element, const Rml::Element* ancestor);
    [[nodiscard]] static SString label(const SUIWidgetInstance& widget);

    // Lua may destroy widgets from inside a hook or an event handler; those are
    // finished once the outermost call returns.
    struct SHookScope
    {
        explicit SHookScope(MViewportSubsystem& owner) : owner(owner) { ++owner.hookDepth; }
        ~SHookScope();
        MViewportSubsystem& owner;
    };
    void processPendingDestroys();

    // -- Hot reload ------------------------------------------------------------------------
    void onAssetLoaded(MAsset* asset);
    void onAssetReloaded(MAsset* asset);
    void rebuildWidget(uint32_t widgetId);
    void onDomainRemoving(MEngineDomain* domain);

    // -- Lookups ---------------------------------------------------------------------------
    [[nodiscard]] SUIWidgetInstance* widgetForRoot(const Rml::Element* element) const;
    [[nodiscard]] static bool isWidgetTag(const Rml::Element* element);
    void logOnce(SUIWidgetInstance& widget, const SString& key, const SString& message);

private:
    // RmlUi
    std::unique_ptr<MRmlSystemInterface>        systemInterface;
    std::unique_ptr<MRmlFileInterface>          fileInterface;
    std::unique_ptr<MRmlRenderInterface>        renderInterface;
    std::unique_ptr<MRmlEventListenerInstancer> eventListenerInstancer;
    std::unique_ptr<MRmlDocumentPlugin>         documentPlugin;
    uint32_t loadingWidgetId = 0;   // the widget buildDocument is loading, 0 otherwise
    std::shared_ptr<Rml::StyleSheetContainer>   baseStyleSheet;
    bool rmlInitialised = false;
    bool rmlFailed      = false;
    bool debuggerInitialised = false;
    bool debuggerVisible     = false;

    // Fonts: GUID -> registered generation; buffers RmlUi still points into.
    std::unordered_map<std::string, uint64_t> registeredFonts;
    std::vector<std::shared_ptr<const std::vector<uint8_t>>> fontBuffers;
    bool fontsRegistered = false;

    // Viewports
    std::vector<std::unique_ptr<MViewport>> viewports;
    uint32_t nextViewportId = 1;

    // Widgets: slot index == widget id - 1. Generations survive slot reuse.
    std::vector<std::unique_ptr<SUIWidgetInstance>> instances;
    std::vector<uint32_t> slotGenerations;
    std::vector<uint32_t> freeSlots;
    std::unordered_map<const Rml::Element*, uint32_t> rootToWidget;
    uint64_t nextAddSequence = 0;

    uint64_t currentDispatchSerial = 0;
    uint64_t nextDispatchSerial    = 0;

    int hookDepth = 0;
    std::vector<uint32_t> pendingDestroys;

    SScopedCallbackDelegateHandle domainRemovingSubscription;
    SScopedCallbackDelegateHandle assetLoadedSubscription;
    SScopedCallbackDelegateHandle assetReloadedSubscription;
};

#endif // VIEWPORT_SUBSYSTEM_H
