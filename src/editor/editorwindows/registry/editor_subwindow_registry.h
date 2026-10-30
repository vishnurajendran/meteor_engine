//
// editor_subwindow_registry.h
//
// Every editor window registers itself here, so adding a window needs no
// change to MEditorApplication:
//
//   // editormemorywindow.cpp
//   EDITOR_REGISTER_SUBWINDOW("Memory", "Window/Debug/Memory", MEditorMemoryWindow)
//
// The registry
// - creates one instance of each window when the editor starts (closed ones
//   stay alive, hidden, so they keep their state - the console keeps logging);
// - keys each window by its MTypeInfo name, which is also its ImGui ID
//   ("Memory###MEditorMemoryWindow"), so layouts survive renames of titles;
// - adds a checkable entry for each window under its menu path, plus
//   Window/Layouts/... to save, load and reset layouts;
// - saves the open windows and ImGui's dock state as a layout:
//     <project>/.engine_data/editor_layouts/<name>.layout   (user layouts)
//     <project>/.engine_data/editor_layouts/_session.layout (autosaved, restored on start)
//     <engine>/editor_layouts/default.layout                (shipped default)
//   With none of those, every registered window opens.
//

#ifndef METEOR_ENGINE_EDITOR_SUBWINDOW_REGISTRY_H
#define METEOR_ENGINE_EDITOR_SUBWINDOW_REGISTRY_H

#include <functional>
#include <vector>

#include "core/memory/mnew.h"
#include "core/object/objectpointer.h"
#include "core/utils/sstring.h"
#include "editor/window/imgui/imguisubwindow.h"

struct SEditorSubWindowDesc
{
    SString typeName;   // MTypeInfo name: unique key, ImGui ID, layout key
    SString title;      // tab / title bar text
    SString menuPath;   // e.g. "Window/Debug/Memory"
    std::function<MImGuiSubWindow*()> create;
};

class MEditorSubWindowRegistry
{
public:
    // Static-init registration (used by EDITOR_REGISTER_SUBWINDOW). Only stores
    // the description; nothing is created until init().
    template<typename T>
    static bool registerWindow(const char* title, const char* menuPath)
    {
        return registerWindow(SEditorSubWindowDesc{
            T::staticTypeInfo().name, title, menuPath,
            []() -> MImGuiSubWindow* { return mnew<T>(); } });
    }
    static bool registerWindow(SEditorSubWindowDesc desc);

    // After the ImGui context exists and before MMenubarTreeNode::buildTree():
    // creates the windows, adds their menu items and restores the last layout.
    static void init();

    // Before ImGui's NewFrame: applies a layout requested from a menu and
    // autosaves the session when something changed.
    static void beginFrame();

    // Writes the session layout. Call while the ImGui context is still alive.
    static void saveSession();

    // Destroys the windows. Call after the editor window has closed.
    static void shutdown();

    // -- Windows -------------------------------------------------------------
    static const std::vector<SEditorSubWindowDesc>& getDescriptors();
    static MImGuiSubWindow* find(const SString& typeName);

    template<typename T>
    static T* get() { return dynamic_cast<T*>(find(T::staticTypeInfo().name)); }

    static bool isOpen(const SString& typeName);
    static void setOpen(const SString& typeName, bool open);

    // -- Layouts -------------------------------------------------------------
    static bool saveLayout(const SString& name);
    static void requestLoadLayout(const SString& name); // applied next frame
    static void requestDefaultLayout();                 // applied next frame
    static std::vector<SString> listLayouts();          // user layouts, by name

    // Marks the session as changed so it gets autosaved.
    static void markDirty() { dirty = true; }

private:
    struct SLayout
    {
        std::vector<SString> openWindows;
        std::string          imguiIni;
    };

    static std::vector<SEditorSubWindowDesc>& descriptors();

    static bool readLayout(const SString& path, SLayout& out);
    static bool writeLayout(const SString& path);
    static void applyLayout(const SLayout& layout);
    static bool loadDefaultLayout();
    static SString layoutPath(const SString& name);

    static std::vector<MObjectPtr<MImGuiSubWindow>> windows;
    static bool    initialised;
    static bool    dirty;
    static bool    hasPendingLayout;
    static SLayout pendingLayout;
};

// Registers an editor window type. Put it in the window's .cpp file.
//   EDITOR_REGISTER_SUBWINDOW("Memory", "Window/Debug/Memory", MEditorMemoryWindow)
#define EDITOR_REGISTER_SUBWINDOW(Title, MenuPath, Type)                                  \
    namespace                                                                            \
    {                                                                                    \
        [[maybe_unused]] const bool _meteorSubWindowRegistered_##Type =                  \
            MEditorSubWindowRegistry::registerWindow<Type>(Title, MenuPath);             \
    }

#endif // METEOR_ENGINE_EDITOR_SUBWINDOW_REGISTRY_H
