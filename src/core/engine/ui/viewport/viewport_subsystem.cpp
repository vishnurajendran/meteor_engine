//
// viewport_subsystem.cpp
//

#include <GL/glew.h>

#include "viewport_subsystem.h"

#include <algorithm>
#include <string>

#include <RmlUi/Core.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#if METEOR_EDITOR
#include <RmlUi/Debugger.h>
#endif

#include "core/application/application.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/domain/engine_domain_subsystem.h"
#include "core/engine/font/font_asset.h"
#include "core/engine/scripting/bridge/dyn_value.h"
#include "core/engine/scripting/interface/script_asset.h"
#include "core/engine/scripting/interface/script_instance.h"
#include "core/engine/scripting/interface/scripting_engine_interface.h"
#include "core/engine/scripting/lua/lua_script_instance.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/textureasset.h"
#include "core/engine/ui/assets/ui_structure_asset.h"
#include "core/engine/ui/assets/ui_style_asset.h"
#include "core/engine/ui/assets/ui_widget_asset.h"
#include "core/engine/ui/rml/rml_event_listener.h"
#include "core/engine/ui/rml/rml_file_interface.h"
#include "core/engine/ui/rml/rml_gl3_output.h"
#include "core/engine/ui/rml/rml_document_plugin.h"
#include "core/engine/ui/rml/rml_input.h"
#include "core/engine/ui/rml/rml_render_interface.h"
#include "core/engine/ui/rml/rml_system_interface.h"
#include "core/engine/ui/ui_event.h"
#include "core/profiling/default_profile_keys.h"
#include "core/profiling/simple_profiler/simple_profiler.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"

namespace
{
    // In front of every document's own sheet, so a widget renders sensibly
    // with no styles of its own. Anything here can be overridden.
    constexpr const char* BASE_STYLE_SHEET = R"RCSS(
body
{
    width: 100%;
    height: 100%;
    font-family: "Meteor Sans";
    font-size: 16dp;
}

widget, div, p, h1, h2, h3, h4, h5, h6
{
    display: block;
}
)RCSS";

    constexpr const char* HOOK_ON_CREATE       = "onCreate";
    constexpr const char* HOOK_ON_ADD          = "onAddToViewport";
    constexpr const char* HOOK_ON_REMOVE       = "onRemove";
    constexpr const char* HOOK_ON_WIDGET_TICK  = "onWidgetTick";

    IAssetManagerSubsystem* assetManager()
    {
        return MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    }

    template <typename F>
    void forEachChild(Rml::Element* element, F&& fn)
    {
        const int count = element->GetNumChildren();
        for (int i = 0; i < count; ++i)
            if (Rml::Element* child = element->GetChild(i))
                fn(child);
    }
}

// =====================================================================================
// Lifetime
// =====================================================================================

MViewportSubsystem::MViewportSubsystem() = default;

MViewportSubsystem::~MViewportSubsystem()
{
    cleanup();
}

void MViewportSubsystem::init()
{
    // RmlUi itself starts on first use (ensureRml): it needs a GL context,
    // which the editor only has after its subsystems are registered.
    if (auto* domains = MEngineSubsystemRegistry::getSubsystem<MEngineDomainSubsystem>())
        domainRemovingSubscription = domains->onDomainRemoving.add([this](MEngineDomain* domain) { onDomainRemoving(domain); });

    assetLoadedSubscription   = MAssetManager::onAssetLoaded().add([this](MAsset* asset) { onAssetLoaded(asset); });
    assetReloadedSubscription = MAssetManager::onAssetReloaded().add([this](MAsset* asset) { onAssetReloaded(asset); });

    MLOG("MViewportSubsystem:: initialised");
}

void MViewportSubsystem::cleanup()
{
    domainRemovingSubscription.invalidate();
    assetLoadedSubscription.invalidate();
    assetReloadedSubscription.invalidate();

    // Behaviours get onRemove like on Stop; the script domains still exist.
    for (size_t slot = 0; slot < instances.size(); ++slot)
    {
        SUIWidgetInstance* widget = instances[slot].get();
        if (!widget || !widget->isTopLevel())
            continue;
        const uint32_t id = widget->id;
        if (widget->inViewport)
            callOnRemoveTree(id);
        destroyTree(id);
    }
    pendingDestroys.clear();

    shutdownRml();
    viewports.clear();
    instances.clear();
    slotGenerations.clear();
    freeSlots.clear();
    rootToWidget.clear();
}

// =====================================================================================
// RmlUi
// =====================================================================================

bool MViewportSubsystem::ensureRml()
{
    if (rmlInitialised)
        return true;
    if (rmlFailed)
        return false;

    // GLEW is initialised with the first window. Before that, try again later.
    if (glCreateShader == nullptr)
        return false;

    Rml::String message;
    if (!RmlGL3::Initialize(&message))
    {
        MERROR("UI: RmlUi's OpenGL 3 renderer could not start: " + SString(message));
        rmlFailed = true;
        return false;
    }

    renderInterface = std::make_unique<MRmlRenderInterface>();
    if (!*renderInterface)
    {
        MERROR("UI: RmlUi's OpenGL 3 renderer failed to compile its shaders - game UI is disabled");
        renderInterface.reset();
        rmlFailed = true;
        return false;
    }

    systemInterface = std::make_unique<MRmlSystemInterface>();
    fileInterface   = std::make_unique<MRmlFileInterface>();
    Rml::SetSystemInterface(systemInterface.get());
    Rml::SetFileInterface(fileInterface.get());
    Rml::SetRenderInterface(renderInterface.get());

    if (!Rml::Initialise())
    {
        MERROR("UI: RmlUi failed to initialise - game UI is disabled");
        renderInterface.reset();
        rmlFailed = true;
        return false;
    }

    // on<event> attributes call behaviour methods instead of inline script.
    eventListenerInstancer = std::make_unique<MRmlEventListenerInstancer>();
    Rml::Factory::RegisterEventListenerInstancer(eventListenerInstancer.get());

    baseStyleSheet = Rml::Factory::InstanceStyleSheetString(BASE_STYLE_SHEET);

    documentPlugin = std::make_unique<MRmlDocumentPlugin>([this](Rml::ElementDocument* document) { onDocumentLoad(document); });
    Rml::RegisterPlugin(documentPlugin.get());
    rmlInitialised = true;
    MLOG("UI: RmlUi initialised");

    registerAllFonts();
    return true;
}

void MViewportSubsystem::shutdownRml()
{
    if (!rmlInitialised)
        return;

#if METEOR_EDITOR
    if (debuggerInitialised)
        Rml::Debugger::Shutdown();
#endif
    debuggerInitialised = false;
    debuggerVisible     = false;

    for (auto& viewport : viewports)
        viewport->destroyContext();

    baseStyleSheet.reset();
    if (documentPlugin)
        Rml::UnregisterPlugin(documentPlugin.get());
    Rml::Shutdown();
    RmlGL3::Shutdown();

    // After Shutdown: RmlUi renders from these until then.
    renderInterface.reset();
    eventListenerInstancer.reset();
    documentPlugin.reset();
    fileInterface.reset();
    systemInterface.reset();
    fontBuffers.clear();
    registeredFonts.clear();
    fontsRegistered = false;
    rmlInitialised  = false;
}

// =====================================================================================
// Fonts
// =====================================================================================

bool MViewportSubsystem::isDefaultFontPath(const SString& path)
{
    return path.startsWith(DEFAULT_FONT_FOLDER) && FileIO::getFileName(path).startsWith(DEFAULT_FONT_PREFIX);
}

void MViewportSubsystem::registerAllFonts()
{
    if (fontsRegistered)
        return;
    fontsRegistered = true;

    // Every font the game can use: in a published game listAssetPaths() is the
    // whole package. Documents name fonts by family only, so the player's
    // on-demand loading would otherwise never load them.
    auto* manager = dynamic_cast<MAssetManager*>(assetManager());
    if (!manager)
        return;
    for (const SString& path : manager->listAssetPaths())
    {
        const SString extension = FileIO::getFileExtension(path).toLower();
        if (extension != "ttf" && extension != "otf")
            continue;
        registerFont(manager->getAsset<MFontAsset>(path).get());
    }
}

void MViewportSubsystem::registerFont(MFontAsset* font)
{
    if (!rmlInitialised || !font || !font->isValid())
        return;

    const std::string key = font->getAssetId().empty() ? font->getPath().str() : font->getAssetId().str();
    if (const auto it = registeredFonts.find(key); it != registeredFonts.end() && it->second == font->getGeneration())
        return;

    // The engine's Open Sans is "Meteor Sans" whatever the files call
    // themselves, so documents never depend on the font's real name. Its
    // regular face is the fallback for glyphs other fonts lack.
    const bool    isDefault = isDefaultFontPath(font->getPath());
    const SString family    = isDefault ? SString(DEFAULT_FONT_FAMILY) : font->getFamily();
    const bool    fallback  = font->isFallback() || (isDefault && font->getWeight() == 400 && !font->isItalic());

    const auto data = font->getSharedData();
    if (!data || data->empty())
        return;

    // RmlUi can't unload one face: an edited font registers again under the
    // same family, and the old bytes stay alive until RmlUi shuts down.
    const bool loaded = Rml::LoadFontFace(
        Rml::Span<const Rml::byte>(reinterpret_cast<const Rml::byte*>(data->data()), data->size()),
        family.str(),
        font->isItalic() ? Rml::Style::FontStyle::Italic : Rml::Style::FontStyle::Normal,
        static_cast<Rml::Style::FontWeight>(font->getWeight()),
        fallback);
    if (!loaded)
    {
        MWARN("UI: RmlUi could not load the font " + font->getPath());
        return;
    }

    fontBuffers.push_back(data);
    registeredFonts[key] = font->getGeneration();
    MVERBOSE(SString::format("UI: font '{0}' {1} registered from {2}", family, font->getStyleName(), font->getPath()));
}

// =====================================================================================
// Viewports
// =====================================================================================

MViewport* MViewportSubsystem::createViewport(IViewportHost* host)
{
    viewports.push_back(std::make_unique<MViewport>(nextViewportId++, host));
    return viewports.back().get();
}

void MViewportSubsystem::destroyViewport(uint32_t viewportId)
{
    for (size_t slot = 0; slot < instances.size(); ++slot)
    {
        SUIWidgetInstance* widget = instances[slot].get();
        if (widget && widget->isTopLevel() && widget->viewportId == viewportId)
            destroyTree(widget->id);
    }
    std::erase_if(viewports, [viewportId](const std::unique_ptr<MViewport>& viewport)
    {
        return viewport->getId() == viewportId;
    });
}

MViewport* MViewportSubsystem::getMainViewport() const
{
    return viewports.empty() ? nullptr : viewports.front().get();
}

MViewport* MViewportSubsystem::getViewport(uint32_t viewportId) const
{
    for (const auto& viewport : viewports)
        if (viewport->getId() == viewportId)
            return viewport.get();
    return nullptr;
}

// =====================================================================================
// Frame
// =====================================================================================

void MViewportSubsystem::update(float deltaTime)
{
    if (!rmlInitialised)
        return;

    const MApplication* app = MApplication::getAppInstance();
    const bool paused = app && app->isPaused();

    // Behaviours first, so the context update below lays out what they set.
    for (const auto& viewport : viewports)
    {
        std::vector<uint32_t> ids;
        for (const auto& entry : viewport->getWidgets())
            ids.push_back(entry.widgetId);
        for (uint32_t id : ids)
            tickTree(id, deltaTime, paused);
    }

    START_PROFILING_SAMPLE(DefaultProfileKeys::UI_UPDATE)
    for (const auto& viewport : viewports)
    {
        if (Rml::Context* context = viewport->getContext())
        {
            context->Update();
            viewport->consumeNeedsUpdate();
        }
    }
    STOP_PROFILING_SAMPLE(DefaultProfileKeys::UI_UPDATE)
}

void MViewportSubsystem::renderViewport(MViewport& viewport, unsigned int framebuffer, int width, int height)
{
    if (!rmlInitialised || !renderInterface || !viewport.getContext())
        return;
    if (!viewport.hasWidgets() && !debuggerVisible)
        return;

    START_PROFILING_SAMPLE(DefaultProfileKeys::UI_RENDER)
    viewport.setSize(width, height);
    // Input or a script changed something after this frame's update.
    if (viewport.consumeNeedsUpdate())
        viewport.getContext()->Update();

    RmlGL3_SetOutputFramebuffer(framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // The renderer saves the GL state it touches in BeginFrame and restores
    // it in EndFrame.
    renderInterface->SetViewport(width, height);
    renderInterface->BeginFrame();
    viewport.getContext()->Render();
    renderInterface->EndFrame();

    RmlGL3_SetOutputFramebuffer(0);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    STOP_PROFILING_SAMPLE(DefaultProfileKeys::UI_RENDER)
}

// =====================================================================================
// Widget table
// =====================================================================================

SUIWidgetInstance* MViewportSubsystem::allocateInstance()
{
    uint32_t slot = 0;
    if (!freeSlots.empty())
    {
        slot = freeSlots.back();
        freeSlots.pop_back();
    }
    else
    {
        slot = static_cast<uint32_t>(instances.size());
        instances.emplace_back();
        slotGenerations.push_back(0);
    }

    instances[slot] = std::make_unique<SUIWidgetInstance>();
    SUIWidgetInstance* widget = instances[slot].get();
    widget->id         = slot + 1;
    widget->generation = ++slotGenerations[slot];
    return widget;
}

void MViewportSubsystem::freeInstance(uint32_t widgetId)
{
    if (widgetId == 0 || widgetId > instances.size() || !instances[widgetId - 1])
        return;
    instances[widgetId - 1].reset();
    freeSlots.push_back(widgetId - 1);
}

SUIWidgetInstance* MViewportSubsystem::getInstance(uint32_t widgetId) const
{
    if (widgetId == 0 || widgetId > instances.size())
        return nullptr;
    return instances[widgetId - 1].get();
}

SUIWidgetInstance* MViewportSubsystem::resolve(const MUIWidget& handle) const
{
    SUIWidgetInstance* widget = getInstance(handle.getWidgetId());
    if (!widget || widget->generation != handle.getGeneration() || widget->viewportId != handle.getViewportId())
        return nullptr;
    return widget->destroyPending ? nullptr : widget;
}

SUIWidgetInstance* MViewportSubsystem::widgetForRoot(const Rml::Element* element) const
{
    const auto it = rootToWidget.find(element);
    return it != rootToWidget.end() ? getInstance(it->second) : nullptr;
}

SUIWidgetInstance* MViewportSubsystem::getTopLevel(const SUIWidgetInstance& widget) const
{
    const SUIWidgetInstance* current = &widget;
    while (current && !current->isTopLevel())
        current = getInstance(current->parentId);
    return const_cast<SUIWidgetInstance*>(current);
}

bool MViewportSubsystem::isShown(const SUIWidgetInstance& widget) const
{
    const SUIWidgetInstance* top = getTopLevel(widget);
    return top && top->inViewport;
}

bool MViewportSubsystem::isWidgetTag(const Rml::Element* element)
{
    return element && element->GetTagName() == "widget";
}

bool MViewportSubsystem::isDescendantOf(const Rml::Element* element, const Rml::Element* ancestor)
{
    for (const Rml::Element* current = element; current; current = current->GetParentNode())
        if (current == ancestor)
            return true;
    return false;
}

SString MViewportSubsystem::label(const SUIWidgetInstance& widget)
{
    return SString("UI widget '") + widget.name + "'";
}

void MViewportSubsystem::logOnce(SUIWidgetInstance& widget, const SString& key, const SString& message)
{
    if (widget.reportedMissingMethods.insert(key).second)
        MWARN(message);
}

std::vector<SString> MViewportSubsystem::chainFor(const SUIWidgetInstance& widget) const
{
    std::vector<SString> chain;
    for (const SUIWidgetInstance* current = &widget; current; current = getInstance(current->parentId))
    {
        chain.push_back(current->asset.getAssetId());
        if (current->isTopLevel())
            break;
    }
    std::reverse(chain.begin(), chain.end());
    return chain;
}

// =====================================================================================
// Creating widgets
// =====================================================================================

MUIWidget MViewportSubsystem::createWidget(const TAssetHandle<MUIWidgetAsset>& asset)
{
    MUIWidgetAsset* widgetAsset = asset.get();
    if (!widgetAsset)
    {
        MERROR("MUIWidget.create: not a UI widget asset (.muia)");
        return {};
    }

#if METEOR_EDITOR
    // Widgets belong to Play; the editor's own domain never shows UI.
    const MApplication* app = MApplication::getAppInstance();
    if (app && !app->isSimulating() && !app->isPaused())
    {
        MWARN("MUIWidget.create: widgets can only be created while playing - " + widgetAsset->getPath());
        return {};
    }
#endif

    MViewport* viewport = getMainViewport();
    if (!viewport)
    {
        MERROR("MUIWidget.create: there is no viewport to create widgets in");
        return {};
    }
    if (!ensureRml() || !viewport->ensureContext(renderInterface.get()))
        return {};

    auto* domains = MEngineSubsystemRegistry::getSubsystem<MEngineDomainSubsystem>();

    SUIWidgetInstance* widget = allocateInstance();
    widget->viewportId     = viewport->getId();
    widget->asset          = asset;
    widget->name           = widgetAsset->getWidgetName();
    widget->domain         = domains ? domains->getCurrentActiveDomain() : nullptr;
    widget->tickWhenPaused = widgetAsset->getTickWhenPaused();
    const uint32_t id = widget->id;

    if (!buildDocument(*widget))
    {
        destroyTree(id);
        return {};
    }

    // Every behaviour exists before any onCreate, so a parent's onCreate can
    // use its children's behaviours (and children see each other's).
    createBehaviours(id);
    {
        SHookScope scope(*this);
        callOnCreateTree(id);
    }

    const SUIWidgetInstance* created = getInstance(id);
    return created && !created->destroyPending ? created->handle() : MUIWidget();
}

bool MViewportSubsystem::buildDocument(SUIWidgetInstance& widget)
{
    MUIWidgetAsset*    widgetAsset = widget.asset.get();
    MUIStructureAsset* structure   = widgetAsset ? widgetAsset->getStructure().get() : nullptr;
    MViewport*         viewport    = getViewport(widget.viewportId);
    if (!structure || !viewport || !viewport->getContext())
    {
        MERROR(label(widget) + ": its structure (.rml) is missing - check the .muia");
        return false;
    }

    // RmlUi reads it through the file interface, so <link>ed sheets and
    // relative paths resolve against the .rml's own path. onDocumentLoad styles
    // it before RmlUi's first update of it.
    widget.ownStyleSheet.reset();
    loadingWidgetId = widget.id;
    Rml::ElementDocument* document = viewport->getContext()->LoadDocument(structure->getPath().str());
    loadingWidgetId = 0;
    if (!document)
    {
        MERROR(label(widget) + ": RmlUi could not load " + structure->getPath());
        return false;
    }

    widget.document = document;
    widget.root     = document;
    rootToWidget[document] = widget.id;
    document->SetClass((SString("w-") + widget.name).str(), true);
    ++widget.domVersion;

    // The base sheet first, then the document's own <link> / <style>; the
    // widgets' styles are combined on top in applyStyles. Normally already
    // done by onDocumentLoad (the document's container is no longer its own
    // after that).
    if (!widget.ownStyleSheet)
        widget.ownStyleSheet = makeOwnStyleSheet(document);

    const uint32_t id = widget.id;
    std::vector<SString> chain{ widget.asset.getAssetId() };
    expandNestedWidgets(id, document, chain);

    if (SUIWidgetInstance* built = getInstance(id))
        applyStyles(*built);
    return true;
}

std::shared_ptr<Rml::StyleSheetContainer> MViewportSubsystem::makeOwnStyleSheet(const Rml::ElementDocument* document) const
{
    const Rml::StyleSheetContainer* own = document ? document->GetStyleSheetContainer() : nullptr;
    if (baseStyleSheet && own)
        return baseStyleSheet->CombineStyleSheetContainer(*own);
    return baseStyleSheet;
}

void MViewportSubsystem::onDocumentLoad(Rml::ElementDocument* document)
{
    SUIWidgetInstance* widget = loadingWidgetId ? getInstance(loadingWidgetId) : nullptr;
    if (!widget || !document)
        return;   // not ours (e.g. RmlUi's debugger)

    // The widget's own sheets for now; nested widgets aren't expanded yet, and
    // applyStyles combines everything once they are (before the next update).
    widget->ownStyleSheet = makeOwnStyleSheet(document);
    std::shared_ptr<Rml::StyleSheetContainer> combined = widget->ownStyleSheet;
    std::vector<SString> paths;
    collectStylePaths(*widget, paths);
    for (const SString& path : paths)
        if (const auto sheet = Rml::Factory::InstanceStyleSheetFile(path.str()))
            combined = combined ? combined->CombineStyleSheetContainer(*sheet) : sheet;
    if (combined)
        document->SetStyleSheetContainer(combined);
}

void MViewportSubsystem::expandNestedWidgets(uint32_t ownerId, Rml::Element* subtree, std::vector<SString>& chain)
{
    // <widget> elements that belong to the owner: not inside another nested
    // widget, whose content is that widget's own business. Document order.
    std::vector<Rml::Element*> found;
    std::vector<Rml::Element*> stack;
    auto pushChildren = [&stack](Rml::Element* element)
    {
        for (int i = element->GetNumChildren() - 1; i >= 0; --i)
            if (Rml::Element* child = element->GetChild(i))
                stack.push_back(child);
    };
    pushChildren(subtree);
    while (!stack.empty())
    {
        Rml::Element* element = stack.back();
        stack.pop_back();
        if (rootToWidget.contains(element))
            continue;
        if (isWidgetTag(element))
        {
            found.push_back(element);
            continue;
        }
        pushChildren(element);
    }

    auto* manager = assetManager();
    for (Rml::Element* element : found)
    {
        SUIWidgetInstance* owner = getInstance(ownerId);
        if (!owner || !manager)
            return;

        const SString src(element->GetAttribute<Rml::String>("src", ""));
        if (src.empty())
        {
            MERROR(label(*owner) + ": a <widget> has no src - give it src=\"guid:...\" of a .muia");
            continue;
        }

        const auto childAsset = manager->getAssetFromReference<MUIWidgetAsset>(src);
        MUIWidgetAsset* child = childAsset.get();
        if (!child)
        {
            MERROR(SString::format("{0}: <widget src=\"{1}\"> is not a UI widget (.muia)", label(*owner), src));
            continue;
        }

        if (!IAssetManagerSubsystem::isGuidReference(src))
            logOnce(*owner, SString("path:") + src,
                    SString::format("{0}: <widget src=\"{1}\"> uses a path - use src=\"{2}\" so moving the widget keeps "
                                    "the reference (the .rml's inspector converts them)",
                                    label(*owner), src, childAsset.toRefString()));

        const SString childGuid = childAsset.getAssetId();
        if (std::find(chain.begin(), chain.end(), childGuid) != chain.end())
        {
            SString names;
            for (const SString& guid : chain)
            {
                const auto link = manager->getAssetById<MUIWidgetAsset>(guid);
                names += (link ? link->getWidgetName() : guid) + " -> ";
            }
            names += child->getWidgetName();
            MERROR(SString("UI widget cycle: ") + names + " - the nested <widget> was not created");
            continue;
        }

        SUIWidgetInstance* nested = allocateInstance();
        owner = getInstance(ownerId);
        nested->viewportId     = owner->viewportId;
        nested->asset          = childAsset;
        nested->name           = child->getWidgetName();
        nested->domain         = owner->domain;
        nested->parentId       = ownerId;
        nested->root           = element;
        nested->tickWhenPaused = child->getTickWhenPaused();
        rootToWidget[element] = nested->id;
        element->SetClass((SString("w-") + nested->name).str(), true);
        owner->children.push_back(nested->id);

        chain.push_back(childGuid);
        buildNestedContent(nested->id, chain);
        chain.pop_back();
    }
}

void MViewportSubsystem::buildNestedContent(uint32_t widgetId, std::vector<SString>& chain)
{
    SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget || !widget->root)
        return;

    MUIWidgetAsset*    widgetAsset = widget->asset.get();
    MUIStructureAsset* structure   = widgetAsset ? widgetAsset->getStructure().get() : nullptr;
    if (!structure)
    {
        MERROR(label(*widget) + ": its structure (.rml) is missing - check the .muia");
        return;
    }

    widget->root->SetInnerRML(structure->getBodyContent().str());
    ++widget->domVersion;
    expandNestedWidgets(widgetId, widget->root, chain);
}

void MViewportSubsystem::collectStylePaths(const SUIWidgetInstance& widget, std::vector<SString>& out) const
{
    if (const MUIWidgetAsset* widgetAsset = widget.asset.get())
    {
        for (const auto& style : widgetAsset->getStyles())
        {
            const MUIStyleAsset* sheet = style.get();
            if (!sheet)
            {
                MWARN(label(widget) + ": one of its styles is not a .rcss asset - check the .muia");
                continue;
            }
            if (std::find(out.begin(), out.end(), sheet->getPath()) == out.end())
                out.push_back(sheet->getPath());
        }
    }
    for (uint32_t childId : widget.children)
        if (const SUIWidgetInstance* child = getInstance(childId))
            collectStylePaths(*child, out);
}

void MViewportSubsystem::applyStyles(SUIWidgetInstance& topLevel)
{
    if (!topLevel.document)
        return;

    // A document has one stylesheet: the widget's styles and every nested
    // widget's, each once, in tree order. CSS isn't scoped - generated sheets
    // keep their rules under .w-<Name> so they don't reach other widgets.
    std::vector<SString> paths;
    collectStylePaths(topLevel, paths);
    topLevel.styleSheetPaths = paths;

    std::shared_ptr<Rml::StyleSheetContainer> combined = topLevel.ownStyleSheet;
    for (const SString& path : paths)
    {
        const auto sheet = Rml::Factory::InstanceStyleSheetFile(path.str());
        if (!sheet)
        {
            MERROR(label(topLevel) + ": RmlUi could not read the stylesheet " + path);
            continue;
        }
        combined = combined ? combined->CombineStyleSheetContainer(*sheet) : sheet;
    }

    if (combined)
        topLevel.document->SetStyleSheetContainer(combined);
    if (MViewport* viewport = getViewport(topLevel.viewportId))
        viewport->markNeedsUpdate();
}

// =====================================================================================
// Behaviours and hooks
// =====================================================================================

void MViewportSubsystem::createBehaviours(uint32_t widgetId)
{
    SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget)
        return;
    const std::vector<uint32_t> children = widget->children;
    for (uint32_t childId : children)
        createBehaviours(childId);
    if (SUIWidgetInstance* again = getInstance(widgetId); again && !again->behaviour)
        createBehaviour(*again);
}

void MViewportSubsystem::createBehaviour(SUIWidgetInstance& widget)
{
    MUIWidgetAsset* widgetAsset = widget.asset.get();
    if (!widgetAsset || widgetAsset->getBehaviourRef().empty())
        return;   // a static widget

    MAsset* scriptAsset = widgetAsset->getBehaviour().get();
    auto*   script      = dynamic_cast<IScriptAsset*>(scriptAsset);
    if (!script)
    {
        MERROR(SString::format("{0}: its behaviour '{1}' is not a script", label(widget), widgetAsset->getBehaviourRef()));
        return;
    }

    auto* scripting = MEngineSubsystemRegistry::getSubsystem<IScriptingEngineSubsystem>();
    if (!scripting)
        return;

    widget.behaviour = scripting->createWidgetScriptInstance(script, widget.handle());
    if (!widget.behaviour)
        return;   // logged by the script context

    // Hooks are only called if the script defines them - checked once.
    widget.hasOnCreate        = widget.behaviour->hasFunc(HOOK_ON_CREATE);
    widget.hasOnAddToViewport = widget.behaviour->hasFunc(HOOK_ON_ADD);
    widget.hasOnRemove        = widget.behaviour->hasFunc(HOOK_ON_REMOVE);
    widget.hasOnWidgetTick    = widget.behaviour->hasFunc(HOOK_ON_WIDGET_TICK);
}

void MViewportSubsystem::releaseBehaviour(SUIWidgetInstance& widget)
{
    if (!widget.behaviour)
        return;
    if (auto* scripting = MEngineSubsystemRegistry::getSubsystem<IScriptingEngineSubsystem>())
        scripting->releaseScriptInstance(widget.behaviour);
    else
        delete widget.behaviour;
    widget.behaviour = nullptr;
    widget.hasOnCreate = widget.hasOnAddToViewport = widget.hasOnRemove = widget.hasOnWidgetTick = false;
}

bool MViewportSubsystem::callHook(uint32_t widgetId, const char* hook, bool (SUIWidgetInstance::*flag))
{
    SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget || !widget->behaviour || !(widget->*flag))
        return false;

    SHookScope scope(*this);
    if (auto* lua = dynamic_cast<MLuaScriptInstance*>(widget->behaviour))
        return lua->callMethod(SString(hook), label(*widget));
    SDynValue result;
    return widget->behaviour->callFunc(SString(hook), std::vector<SDynValue>{}, result);
}

bool MViewportSubsystem::callHook(uint32_t widgetId, const char* hook, bool (SUIWidgetInstance::*flag), float value)
{
    SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget || !widget->behaviour || !(widget->*flag))
        return false;

    SHookScope scope(*this);
    if (auto* lua = dynamic_cast<MLuaScriptInstance*>(widget->behaviour))
        return lua->callMethod(SString(hook), label(*widget), value);
    SDynValue argument;
    argument.setFloat(value);
    SDynValue result;
    return widget->behaviour->callFunc(SString(hook), std::vector<SDynValue>{ argument }, result);
}

void MViewportSubsystem::callOnCreateTree(uint32_t widgetId)
{
    const SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget || widget->destroyPending)
        return;
    const std::vector<uint32_t> children = widget->children;
    for (uint32_t childId : children)
        callOnCreateTree(childId);
    callHook(widgetId, HOOK_ON_CREATE, &SUIWidgetInstance::hasOnCreate);
}

void MViewportSubsystem::callOnAddToViewportTree(uint32_t widgetId)
{
    callHook(widgetId, HOOK_ON_ADD, &SUIWidgetInstance::hasOnAddToViewport);
    const SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget)
        return;
    const std::vector<uint32_t> children = widget->children;
    for (uint32_t childId : children)
        callOnAddToViewportTree(childId);
}

void MViewportSubsystem::callOnRemoveTree(uint32_t widgetId)
{
    callHook(widgetId, HOOK_ON_REMOVE, &SUIWidgetInstance::hasOnRemove);
    const SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget)
        return;
    const std::vector<uint32_t> children = widget->children;
    for (uint32_t childId : children)
        callOnRemoveTree(childId);
}

void MViewportSubsystem::tickTree(uint32_t widgetId, float deltaTime, bool paused)
{
    const SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget || widget->destroyPending)
        return;

    // Each widget follows its own tickWhenPaused, nested ones included.
    if (!paused || widget->tickWhenPaused)
        callHook(widgetId, HOOK_ON_WIDGET_TICK, &SUIWidgetInstance::hasOnWidgetTick, deltaTime);

    widget = getInstance(widgetId);
    if (!widget || widget->destroyPending)
        return;
    const std::vector<uint32_t> children = widget->children;
    for (uint32_t childId : children)
        tickTree(childId, deltaTime, paused);
}

MViewportSubsystem::SHookScope::~SHookScope()
{
    if (--owner.hookDepth == 0)
        owner.processPendingDestroys();
}

void MViewportSubsystem::processPendingDestroys()
{
    while (!pendingDestroys.empty())
    {
        const uint32_t id = pendingDestroys.back();
        pendingDestroys.pop_back();
        destroyTree(id);
    }
}

// =====================================================================================
// Viewport membership, visibility, destruction
// =====================================================================================

void MViewportSubsystem::restack(MViewport& viewport)
{
    // z-index decides the order documents draw in; pulling each to the front
    // in order keeps the tree order the same, for ties RmlUi breaks itself.
    int rank = 0;
    for (const auto& entry : viewport.getWidgets())
    {
        SUIWidgetInstance* widget = getInstance(entry.widgetId);
        if (!widget || !widget->document)
            continue;
        widget->document->SetProperty("z-index", std::to_string(rank++));
        widget->document->PullToFront();
    }
    viewport.markNeedsUpdate();
}

void MViewportSubsystem::addToViewport(const MUIWidget& handle, int zOrder)
{
    SUIWidgetInstance* widget = resolve(handle);
    if (!widget)
        return;
    if (!widget->isTopLevel())
    {
        MWARN(label(*widget) + ": addToViewport is for top-level widgets - this one is nested in another widget");
        return;
    }
    MViewport* viewport = getViewport(widget->viewportId);
    if (!viewport)
        return;

    const bool wasShown = widget->inViewport;
    widget->zOrder = zOrder;
    if (!wasShown)
        widget->addSequence = ++nextAddSequence;
    widget->inViewport = true;
    viewport->addWidgetEntry(widget->id, zOrder, widget->addSequence);

    if (widget->visible && widget->document)
        widget->document->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    restack(*viewport);

    if (!wasShown)
    {
        SHookScope scope(*this);
        callOnAddToViewportTree(handle.getWidgetId());
    }
}

void MViewportSubsystem::removeFromViewport(const MUIWidget& handle)
{
    SUIWidgetInstance* widget = resolve(handle);
    if (!widget || !widget->isTopLevel() || !widget->inViewport)
        return;

    widget->inViewport = false;
    if (MViewport* viewport = getViewport(widget->viewportId))
    {
        viewport->removeWidgetEntry(widget->id);
        viewport->markNeedsUpdate();
    }
    if (widget->document)
        widget->document->Hide();

    SHookScope scope(*this);
    callOnRemoveTree(handle.getWidgetId());
}

void MViewportSubsystem::destroyWidget(const MUIWidget& handle)
{
    SUIWidgetInstance* widget = resolve(handle);
    if (!widget)
        return;
    if (!widget->isTopLevel())
    {
        MWARN(label(*widget) + ": a nested widget is destroyed with its parent - destroy the top-level widget instead");
        return;
    }

    const uint32_t id = widget->id;
    if (widget->inViewport)
        removeFromViewport(handle);

    widget = getInstance(id);
    if (!widget || widget->generation != handle.getGeneration())
        return;

    // From inside a script call: invalid now, freed once the call returns.
    if (hookDepth > 0)
    {
        if (!widget->destroyPending)
        {
            widget->destroyPending = true;
            pendingDestroys.push_back(id);
        }
        return;
    }
    destroyTree(id);
}

void MViewportSubsystem::setWidgetVisible(const MUIWidget& handle, bool visible)
{
    SUIWidgetInstance* widget = resolve(handle);
    if (!widget || widget->visible == visible)
        return;
    widget->visible = visible;

    if (widget->isTopLevel())
    {
        if (widget->document && widget->inViewport)
        {
            if (visible)
                widget->document->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
            else
                widget->document->Hide();
        }
    }
    else if (widget->root)
    {
        if (visible)
            widget->root->RemoveProperty("display");
        else
            widget->root->SetProperty("display", "none");
    }

    if (MViewport* viewport = getViewport(widget->viewportId))
        viewport->markNeedsUpdate();
}

void MViewportSubsystem::destroyChildren(uint32_t widgetId)
{
    SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget)
        return;
    const std::vector<uint32_t> children = widget->children;
    for (uint32_t childId : children)
    {
        destroyChildren(childId);
        if (SUIWidgetInstance* child = getInstance(childId))
        {
            releaseBehaviour(*child);
            if (child->root)
                rootToWidget.erase(child->root);
            freeInstance(childId);
        }
    }
    if (SUIWidgetInstance* again = getInstance(widgetId))
        again->children.clear();
}

void MViewportSubsystem::destroyTree(uint32_t widgetId)
{
    destroyChildren(widgetId);
    SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget)
        return;

    releaseBehaviour(*widget);
    if (widget->root)
        rootToWidget.erase(widget->root);

    if (widget->isTopLevel())
    {
        if (MViewport* viewport = getViewport(widget->viewportId))
        {
            viewport->removeWidgetEntry(widget->id);
            viewport->markNeedsUpdate();
        }
        // Closed at the context's next update.
        if (widget->document)
            widget->document->Close();
    }
    else if (SUIWidgetInstance* parent = getInstance(widget->parentId))
    {
        std::erase(parent->children, widgetId);
    }
    freeInstance(widgetId);
}

// =====================================================================================
// Lookups
// =====================================================================================

Rml::Element* MViewportSubsystem::findElement(const SUIWidgetInstance& widget, const SString& elementId) const
{
    if (!widget.root || elementId.empty())
        return nullptr;

    const std::string& id = elementId.str();
    std::vector<Rml::Element*> stack;
    auto pushChildren = [&stack](Rml::Element* element)
    {
        for (int i = element->GetNumChildren() - 1; i >= 0; --i)
            if (Rml::Element* child = element->GetChild(i))
                stack.push_back(child);
    };
    pushChildren(widget.root);
    while (!stack.empty())
    {
        Rml::Element* element = stack.back();
        stack.pop_back();
        if (element->GetId() == id)
            return element;
        // A nested widget's <widget> tag is this widget's element; what is
        // inside it is not.
        if (rootToWidget.contains(element))
            continue;
        pushChildren(element);
    }
    return nullptr;
}

SUIWidgetInstance* MViewportSubsystem::findChildWidget(const SUIWidgetInstance& widget, const SString& widgetId) const
{
    for (uint32_t childId : widget.children)
    {
        SUIWidgetInstance* child = getInstance(childId);
        if (child && !child->destroyPending && child->root && child->root->GetId() == widgetId.str())
            return child;
    }
    return nullptr;
}

void MViewportSubsystem::replaceElementContent(const MUIWidget& ownerHandle, Rml::Element* element, const SString& rml)
{
    SUIWidgetInstance* owner = resolve(ownerHandle);
    if (!owner || !element)
        return;
    const uint32_t ownerId = owner->id;
    const bool     shown   = isShown(*owner);

    // Nested widgets inside the element go with it.
    std::vector<uint32_t> replaced;
    for (uint32_t childId : owner->children)
        if (const SUIWidgetInstance* child = getInstance(childId); child && isDescendantOf(child->root, element))
            replaced.push_back(childId);
    {
        SHookScope scope(*this);
        for (uint32_t childId : replaced)
        {
            if (shown)
                callOnRemoveTree(childId);
            destroyTree(childId);
        }
    }

    owner = getInstance(ownerId);
    if (!owner)
        return;
    element->SetInnerRML(rml.str());
    ++owner->domVersion;

    // <widget> elements in the new content become nested widgets.
    const std::vector<uint32_t> before = owner->children;
    std::vector<SString> chain = chainFor(*owner);
    expandNestedWidgets(ownerId, element, chain);

    owner = getInstance(ownerId);
    if (!owner || owner->children.size() == before.size())
    {
        if (MViewport* viewport = owner ? getViewport(owner->viewportId) : nullptr)
            viewport->markNeedsUpdate();
        return;
    }

    std::vector<uint32_t> added;
    for (uint32_t childId : owner->children)
        if (std::find(before.begin(), before.end(), childId) == before.end())
            added.push_back(childId);

    if (SUIWidgetInstance* top = getTopLevel(*owner))
        applyStyles(*top);

    SHookScope scope(*this);
    for (uint32_t childId : added)
    {
        createBehaviours(childId);
        callOnCreateTree(childId);
        if (shown)
            callOnAddToViewportTree(childId);
    }
}

// =====================================================================================
// Events
// =====================================================================================

void MViewportSubsystem::dispatchElementEvent(Rml::Element* listenerElement, const SString& methodName, Rml::Event& event)
{
    if (!listenerElement)
        return;

    // on<event> on a <widget> tag is written in the enclosing widget's file,
    // so it goes to that widget's behaviour.
    const Rml::Element* start = listenerElement;
    if (isWidgetTag(start) && rootToWidget.contains(start))
        start = start->GetParentNode();

    SUIWidgetInstance* owner = nullptr;
    for (const Rml::Element* element = start; element && !owner; element = element->GetParentNode())
        owner = widgetForRoot(element);
    if (!owner || owner->destroyPending)
        return;   // not a widget's element (e.g. RmlUi's debugger)

    const SString attribute = SString("on") + SString(event.GetType());
    if (!owner->behaviour)
    {
        logOnce(*owner, SString("no-behaviour"),
                SString::format("{0}: {1}=\"{2}\" has nowhere to go - the widget has no behaviour",
                                label(*owner), attribute, methodName));
        return;
    }
    if (!owner->behaviour->hasFunc(methodName))
    {
        logOnce(*owner, SString("method:") + methodName,
                SString::format("{0}: its behaviour has no method '{1}' (from {2} on <{3}>)", label(*owner),
                                methodName, attribute, SString(listenerElement->GetTagName())));
        return;
    }

    SUIEvent uiEvent;
    uiEvent.type = SString(event.GetType());
    if (const Rml::Element* target = event.GetTargetElement())
        uiEvent.targetId = SString(target->GetId());
    if (const Rml::Element* current = event.GetCurrentElement())
        uiEvent.currentId = SString(current->GetId());
    uiEvent.mouseX = event.GetParameter<float>("mouse_x", 0.0f);
    uiEvent.mouseY = event.GetParameter<float>("mouse_y", 0.0f);
    uiEvent.key    = MRmlInput::fromRml(event.GetParameter<int>("key_identifier", 0));
    uiEvent.value  = SString(event.GetParameter<Rml::String>("value", Rml::String()));
    uiEvent.source = &event;
    uiEvent.dispatchSerial = ++nextDispatchSerial;

    const uint32_t viewportId = owner->viewportId;
    const uint64_t previousSerial = currentDispatchSerial;
    currentDispatchSerial = uiEvent.dispatchSerial;
    {
        SHookScope scope(*this);
        if (auto* lua = dynamic_cast<MLuaScriptInstance*>(owner->behaviour))
            lua->callMethod(methodName, label(*owner), uiEvent);
    }
    currentDispatchSerial = previousSerial;

    if (MViewport* viewport = getViewport(viewportId))
        viewport->markNeedsUpdate();
}

// =====================================================================================
// Hot reload and teardown
// =====================================================================================

void MViewportSubsystem::onAssetLoaded(MAsset* asset)
{
    // A font added while running (dropped into the project in the editor).
    if (auto* font = dynamic_cast<MFontAsset*>(asset); font && fontsRegistered)
        registerFont(font);
}

void MViewportSubsystem::onAssetReloaded(MAsset* asset)
{
    if (!rmlInitialised || !asset)
        return;

    if (auto* font = dynamic_cast<MFontAsset*>(asset))
    {
        registerFont(font);
        return;
    }

    if (dynamic_cast<MTextureAsset*>(asset))
    {
        // RmlUi asks for every texture again; asset textures may have a new GL id.
        Rml::ReleaseTextures();
        if (renderInterface)
            renderInterface->clearAssetTextures();
        for (const auto& viewport : viewports)
            viewport->markNeedsUpdate();
        return;
    }

    if (auto* style = dynamic_cast<MUIStyleAsset*>(asset))
    {
        // Stylesheets are reapplied in place: documents and behaviour state stay.
        Rml::Factory::ClearStyleSheetCache();
        for (const auto& slot : instances)
        {
            SUIWidgetInstance* widget = slot.get();
            if (widget && widget->isTopLevel() &&
                std::find(widget->styleSheetPaths.begin(), widget->styleSheetPaths.end(), style->getPath()) != widget->styleSheetPaths.end())
                applyStyles(*widget);
        }
        return;
    }

    // Structure or widget edits rebuild every instance using them.
    const auto* structure   = dynamic_cast<MUIStructureAsset*>(asset);
    const auto* widgetAsset = dynamic_cast<MUIWidgetAsset*>(asset);
    if (!structure && !widgetAsset)
        return;

    Rml::Factory::ClearStyleSheetCache();
    std::vector<MUIWidget> affected;
    for (const auto& slot : instances)
    {
        const SUIWidgetInstance* widget = slot.get();
        if (!widget || widget->destroyPending)
            continue;
        bool uses = false;
        if (widgetAsset)
            uses = widget->asset.getAssetId() == asset->getAssetId();
        else if (const MUIWidgetAsset* owner = widget->asset.get())
            uses = owner->getStructure().getAssetId() == asset->getAssetId();
        if (uses)
            affected.push_back(widget->handle());
    }
    // Top-level first: rebuilding one recreates its nested widgets anyway.
    std::stable_sort(affected.begin(), affected.end(), [this](const MUIWidget& a, const MUIWidget& b)
    {
        const SUIWidgetInstance* wa = resolve(a);
        const SUIWidgetInstance* wb = resolve(b);
        return (wa && wa->isTopLevel()) && !(wb && wb->isTopLevel());
    });
    for (const MUIWidget& handle : affected)
        if (resolve(handle))
            rebuildWidget(handle.getWidgetId());
}

void MViewportSubsystem::rebuildWidget(uint32_t widgetId)
{
    SUIWidgetInstance* widget = getInstance(widgetId);
    if (!widget)
        return;
    const MUIWidget handle = widget->handle();
    const bool shown = isShown(*widget);
    MLOG(SString("UI: rebuilding ") + label(*widget));

    // Nested widgets are made again, with new behaviours.
    {
        SHookScope scope(*this);
        if (shown)
            for (uint32_t childId : std::vector<uint32_t>(widget->children))
                callOnRemoveTree(childId);
    }
    if (!resolve(handle))
        return;
    destroyChildren(widgetId);
    widget = getInstance(widgetId);

    if (widget->isTopLevel())
    {
        if (widget->document)
        {
            rootToWidget.erase(widget->document);
            widget->document->Close();
        }
        widget->document = nullptr;
        widget->root     = nullptr;
        if (!buildDocument(*widget))
        {
            MERROR(label(*widget) + ": could not be rebuilt - it stays empty until the file is fixed");
            return;
        }
        widget = getInstance(widgetId);
        if (widget->inViewport && widget->visible)
            widget->document->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
        if (MViewport* viewport = getViewport(widget->viewportId))
            restack(*viewport);
    }
    else
    {
        std::vector<SString> chain = chainFor(*widget);
        buildNestedContent(widgetId, chain);
        if (SUIWidgetInstance* top = getTopLevel(*getInstance(widgetId)))
            applyStyles(*top);
    }

    // The widget's own behaviour stays and gets onCreate again (it must find
    // its elements again); nested ones are new.
    createBehaviours(widgetId);
    SHookScope scope(*this);
    callOnCreateTree(widgetId);
    if (shown)
        if (const SUIWidgetInstance* rebuilt = getInstance(widgetId))
            for (uint32_t childId : std::vector<uint32_t>(rebuilt->children))
                callOnAddToViewportTree(childId);
}

void MViewportSubsystem::onDomainRemoving(MEngineDomain* domain)
{
    // Stop: every widget the game created goes, onRemove first.
    std::vector<MUIWidget> owned;
    for (const auto& slot : instances)
        if (const SUIWidgetInstance* widget = slot.get(); widget && widget->isTopLevel() && widget->domain == domain)
            owned.push_back(widget->handle());

    {
        SHookScope scope(*this);
        for (const MUIWidget& handle : owned)
            if (SUIWidgetInstance* widget = resolve(handle); widget && widget->inViewport)
                removeFromViewport(handle);
    }
    for (const MUIWidget& handle : owned)
        if (SUIWidgetInstance* widget = getInstance(handle.getWidgetId()); widget && widget->generation == handle.getGeneration())
            destroyTree(widget->id);

    for (const auto& viewport : viewports)
        viewport->resetCursor();
    if (debuggerVisible)
        setDebuggerVisible(false);
}

// =====================================================================================
// Editor
// =====================================================================================

void MViewportSubsystem::setDebuggerVisible(bool visible)
{
#if METEOR_EDITOR
    MViewport* viewport = getMainViewport();
    if (!viewport || !ensureRml() || !viewport->ensureContext(renderInterface.get()))
        return;
    if (!debuggerInitialised)
    {
        debuggerInitialised = Rml::Debugger::Initialise(viewport->getContext());
        if (!debuggerInitialised)
        {
            MERROR("UI: RmlUi's debugger could not start");
            return;
        }
    }
    Rml::Debugger::SetVisible(visible);
    debuggerVisible = visible;
    viewport->markNeedsUpdate();
#else
    (void)visible;
#endif
}
