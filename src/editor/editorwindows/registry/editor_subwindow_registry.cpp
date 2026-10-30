#include "editor_subwindow_registry.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

#include "imgui.h"
#include "pugixml.hpp"

#include "core/utils/logger.h"
#include "core/utils/meteor_paths.h"
#include "editor/window/menubar/menubaritem.h"
#include "editor/window/menubar/menubartree.h"

namespace
{
    constexpr const char* LAYOUT_DIR       = ".engine_data/editor_layouts/";
    constexpr const char* LAYOUT_EXTENSION = ".layout";
    constexpr const char* SESSION_LAYOUT   = "_session";             // leading '_' hides it from the list
    constexpr const char* DEFAULT_LAYOUT   = "editor_layouts/default.layout"; // under ENGINE_PATH
    constexpr double      AUTOSAVE_DELAY   = 2.0;                    // seconds between session saves

    double lastSaveTime = 0.0;
    std::vector<bool> lastOpenState;

    // Keeps letters, digits, space, '-' and '_' so the name is a safe file name.
    SString sanitiseLayoutName(const std::string& raw)
    {
        std::string name;
        for (const char c : raw)
        {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '_')
                name += c;
        }
        while (!name.empty() && name.back() == ' ')  name.pop_back();
        while (!name.empty() && name.front() == ' ') name.erase(name.begin());
        return SString(name);
    }
}

// -- Menu items -----------------------------------------------------------------

// One per registered window: toggles it, with a check mark while it is open.
class MEditorSubWindowMenuItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MEditorSubWindowMenuItem)
public:
    MEditorSubWindowMenuItem(SString typeName, SString path, const int priority)
        : typeName(std::move(typeName)), path(std::move(path)), priority(priority) {}

    [[nodiscard]] int     getPriority() const override { return priority; }
    [[nodiscard]] SString getPath()     const override { return path; }
    [[nodiscard]] bool    isChecked()   const override { return MEditorSubWindowRegistry::isOpen(typeName); }

    void onSelect() override
    {
        MEditorSubWindowRegistry::setOpen(typeName, !MEditorSubWindowRegistry::isOpen(typeName));
    }

private:
    SString typeName;
    SString path;
    int     priority;
};

class MSaveLayoutMenuItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MSaveLayoutMenuItem)
public:
    [[nodiscard]] int     getPriority() const override { return MENU_PRIORITY_BASE_WINDOW + 900; }
    [[nodiscard]] SString getPath()     const override { return "Window/Layouts/Save Layout..."; }
    void onSelect() override { openRequested = true; }

    void drawPopup() override
    {
        static constexpr const char* POPUP_ID = "Save Layout##MeteorSaveLayout";
        if (openRequested)
        {
            ImGui::OpenPopup(POPUP_ID);
            openRequested = false;
            nameBuffer[0] = '\0';
            error = false;
        }

        if (!ImGui::BeginPopupModal(POPUP_ID, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        const bool submitted = ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer),
                                                ImGuiInputTextFlags_EnterReturnsTrue);
        if (error)
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Enter a name (letters, digits, space, - or _)");

        if (ImGui::Button("Save") || submitted)
        {
            const SString name = sanitiseLayoutName(nameBuffer);
            error = name.empty() || name.str().front() == '_';
            if (!error)
            {
                MEditorSubWindowRegistry::saveLayout(name);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

private:
    bool openRequested = false;
    bool error         = false;
    char nameBuffer[64] = {};
};

class MLoadLayoutMenuItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MLoadLayoutMenuItem)
public:
    [[nodiscard]] int     getPriority() const override { return MENU_PRIORITY_BASE_WINDOW + 901; }
    [[nodiscard]] SString getPath()     const override { return "Window/Layouts/Load Layout..."; }
    void onSelect() override { openRequested = true; }

    void drawPopup() override
    {
        static constexpr const char* POPUP_ID = "Load Layout##MeteorLoadLayout";
        if (openRequested)
        {
            ImGui::OpenPopup(POPUP_ID);
            openRequested = false;
            layouts  = MEditorSubWindowRegistry::listLayouts();
            selected = layouts.empty() ? -1 : 0;
        }

        if (!ImGui::BeginPopupModal(POPUP_ID, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        if (layouts.empty())
            ImGui::TextDisabled("No saved layouts yet. Use Window > Layouts > Save Layout...");

        bool load = false;
        for (int i = 0; i < static_cast<int>(layouts.size()); ++i)
        {
            if (ImGui::Selectable(layouts[i].c_str(), selected == i, ImGuiSelectableFlags_AllowDoubleClick))
            {
                selected = i;
                load = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
            }
        }

        ImGui::BeginDisabled(selected < 0);
        load |= ImGui::Button("Load");
        ImGui::EndDisabled();
        if (load && selected >= 0)
        {
            MEditorSubWindowRegistry::requestLoadLayout(layouts[selected]);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

private:
    bool                 openRequested = false;
    int                  selected      = -1;
    std::vector<SString> layouts;
};

class MResetLayoutMenuItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MResetLayoutMenuItem)
public:
    [[nodiscard]] int     getPriority() const override { return MENU_PRIORITY_BASE_WINDOW + 902; }
    [[nodiscard]] SString getPath()     const override { return "Window/Layouts/Reset to Default"; }
    void onSelect() override { MEditorSubWindowRegistry::requestDefaultLayout(); }
};

// -- Registry -------------------------------------------------------------------

std::vector<MObjectPtr<MImGuiSubWindow>> MEditorSubWindowRegistry::windows;
bool                                      MEditorSubWindowRegistry::initialised      = false;
bool                                      MEditorSubWindowRegistry::dirty            = false;
bool                                      MEditorSubWindowRegistry::hasPendingLayout = false;
MEditorSubWindowRegistry::SLayout         MEditorSubWindowRegistry::pendingLayout;

std::vector<SEditorSubWindowDesc>& MEditorSubWindowRegistry::descriptors()
{
    // Function-local so it exists before any static registration uses it.
    static std::vector<SEditorSubWindowDesc> list;
    return list;
}

const std::vector<SEditorSubWindowDesc>& MEditorSubWindowRegistry::getDescriptors()
{
    return descriptors();
}

bool MEditorSubWindowRegistry::registerWindow(SEditorSubWindowDesc desc)
{
    descriptors().push_back(std::move(desc));
    return true;
}

void MEditorSubWindowRegistry::init()
{
    if (initialised)
        return;

    // The registry saves ImGui's settings inside layouts instead of imgui.ini.
    ImGui::GetIO().IniFilename = nullptr;

    // Menu order follows registration order, which differs between builds;
    // sort by path so the Window menu is stable.
    auto& descs = descriptors();
    std::sort(descs.begin(), descs.end(), [](const auto& a, const auto& b) { return a.menuPath < b.menuPath; });

    windows.reserve(descs.size());
    for (size_t i = 0; i < descs.size(); ++i)
    {
        const auto& desc = descs[i];
        MImGuiSubWindow* window = desc.create();
        window->setTitle(desc.title);
        window->setOpen(false, false);
        windows.emplace_back(window);

        MMenubarTreeNode::registerItem(mnew<MEditorSubWindowMenuItem>(
            desc.typeName, desc.menuPath, MMenubarItem::MENU_PRIORITY_BASE_WINDOW + static_cast<int>(i)));
    }
    MMenubarTreeNode::registerItem(mnew<MSaveLayoutMenuItem>());
    MMenubarTreeNode::registerItem(mnew<MLoadLayoutMenuItem>());
    MMenubarTreeNode::registerItem(mnew<MResetLayoutMenuItem>());

    initialised = true;

    SLayout session;
    if (readLayout(layoutPath(SESSION_LAYOUT), session))
    {
        applyLayout(session);
        MVERBOSE("MEditorSubWindowRegistry:: Restored last session layout");
    }
    else if (!loadDefaultLayout())
    {
        for (auto& window : windows)
            window->setOpen(true, false);
        MLOG("MEditorSubWindowRegistry:: No layout found, opening every window");
    }

    lastOpenState.clear();
    for (const auto& window : windows)
        lastOpenState.push_back(window->isOpen());
    dirty = false;
}

void MEditorSubWindowRegistry::beginFrame()
{
    if (!initialised)
        return;

    if (hasPendingLayout)
    {
        applyLayout(pendingLayout);
        pendingLayout    = {};
        hasPendingLayout = false;
        dirty            = true;
    }

    // Windows closed with their own X button.
    for (size_t i = 0; i < windows.size() && i < lastOpenState.size(); ++i)
    {
        const bool open = windows[i]->isOpen();
        if (open != lastOpenState[i])
        {
            lastOpenState[i] = open;
            dirty = true;
        }
    }

    // ImGui raises WantSaveIniSettings a few seconds after docking/size changes.
    ImGuiIO& io = ImGui::GetIO();
    const double now = ImGui::GetTime();
    if ((dirty || io.WantSaveIniSettings) && now - lastSaveTime >= AUTOSAVE_DELAY)
    {
        saveSession();
        io.WantSaveIniSettings = false;
        lastSaveTime = now;
    }
}

void MEditorSubWindowRegistry::saveSession()
{
    if (!initialised || !ImGui::GetCurrentContext())
        return;
    if (writeLayout(layoutPath(SESSION_LAYOUT)))
        dirty = false;
}

void MEditorSubWindowRegistry::shutdown()
{
    windows.clear();
    lastOpenState.clear();
    initialised = false;
}

MImGuiSubWindow* MEditorSubWindowRegistry::find(const SString& typeName)
{
    const auto& descs = descriptors();
    for (size_t i = 0; i < descs.size() && i < windows.size(); ++i)
    {
        if (descs[i].typeName == typeName)
            return windows[i].get();
    }
    return nullptr;
}

bool MEditorSubWindowRegistry::isOpen(const SString& typeName)
{
    const auto* window = find(typeName);
    return window && window->isOpen();
}

void MEditorSubWindowRegistry::setOpen(const SString& typeName, const bool open)
{
    if (auto* window = find(typeName))
    {
        window->setOpen(open);
        dirty = true;
    }
}

// -- Layouts --------------------------------------------------------------------

SString MEditorSubWindowRegistry::layoutPath(const SString& name)
{
    return PROJECT_PATH(SString(LAYOUT_DIR) + name + LAYOUT_EXTENSION);
}

bool MEditorSubWindowRegistry::saveLayout(const SString& name)
{
    const bool ok = writeLayout(layoutPath(name));
    if (ok) MLOG(SString::format("MEditorSubWindowRegistry:: Saved layout '{0}'", name));
    return ok;
}

void MEditorSubWindowRegistry::requestLoadLayout(const SString& name)
{
    SLayout layout;
    if (!readLayout(layoutPath(name), layout))
    {
        MWARN(SString::format("MEditorSubWindowRegistry:: Could not read layout '{0}'", name));
        return;
    }
    pendingLayout    = std::move(layout);
    hasPendingLayout = true;
}

void MEditorSubWindowRegistry::requestDefaultLayout()
{
    SLayout layout;
    if (readLayout(ENGINE_PATH(DEFAULT_LAYOUT), layout))
    {
        pendingLayout = std::move(layout);
    }
    else
    {
        pendingLayout = {};
        for (const auto& desc : descriptors())
            pendingLayout.openWindows.push_back(desc.typeName);
    }
    hasPendingLayout = true;
}

std::vector<SString> MEditorSubWindowRegistry::listLayouts()
{
    std::vector<SString> names;
    std::error_code ec;
    const std::filesystem::path dir = PROJECT_PATH(LAYOUT_DIR).str();
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file() || entry.path().extension() != LAYOUT_EXTENSION)
            continue;
        const std::string stem = entry.path().stem().string();
        if (!stem.empty() && stem.front() != '_')
            names.emplace_back(stem);
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool MEditorSubWindowRegistry::loadDefaultLayout()
{
    SLayout layout;
    if (!readLayout(ENGINE_PATH(DEFAULT_LAYOUT), layout))
        return false;
    applyLayout(layout);
    MVERBOSE("MEditorSubWindowRegistry:: Applied default layout");
    return true;
}

void MEditorSubWindowRegistry::applyLayout(const SLayout& layout)
{
    const auto& descs = descriptors();
    for (size_t i = 0; i < descs.size() && i < windows.size(); ++i)
    {
        const bool open = std::find(layout.openWindows.begin(), layout.openWindows.end(), descs[i].typeName)
                          != layout.openWindows.end();
        windows[i]->setOpen(open, false);
    }

    if (!layout.imguiIni.empty())
        ImGui::LoadIniSettingsFromMemory(layout.imguiIni.c_str(), layout.imguiIni.size());
}

// <layout version="1">
//   <window type="MEditorSceneViewWindow"/>    (open windows only)
//   <imgui><![CDATA[ ImGui ini text: positions, sizes, dock tree ]]></imgui>
// </layout>
bool MEditorSubWindowRegistry::writeLayout(const SString& path)
{
    pugi::xml_document doc;
    auto root = doc.append_child("layout");
    root.append_attribute("version") = 1;

    const auto& descs = descriptors();
    for (size_t i = 0; i < descs.size() && i < windows.size(); ++i)
    {
        if (windows[i]->isOpen())
            root.append_child("window").append_attribute("type") = descs[i].typeName.c_str();
    }

    size_t iniSize = 0;
    const char* ini = ImGui::SaveIniSettingsToMemory(&iniSize);
    root.append_child("imgui").append_child(pugi::node_cdata).set_value(std::string(ini, iniSize).c_str());

    std::error_code ec;
    const std::filesystem::path file = path.str();
    std::filesystem::create_directories(file.parent_path(), ec);
    if (!doc.save_file(file.string().c_str(), "  "))
    {
        MWARN(SString::format("MEditorSubWindowRegistry:: Could not write layout {0}", path));
        return false;
    }
    return true;
}

bool MEditorSubWindowRegistry::readLayout(const SString& path, SLayout& out)
{
    std::error_code ec;
    if (!std::filesystem::exists(path.str(), ec))
        return false;

    pugi::xml_document doc;
    if (!doc.load_file(path.c_str()))
        return false;

    const auto root = doc.child("layout");
    if (!root)
        return false;

    out = {};
    for (const auto window : root.children("window"))
        out.openWindows.emplace_back(window.attribute("type").as_string());
    out.imguiIni = root.child("imgui").child_value();
    return true;
}
