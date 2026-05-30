#include "editorassetwindow.h"

#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "core/engine/texture/textureasset.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/app/editorapplication.h"
#include "editor/window/menubar/menubartree.h"

#include "imgui-SFML.h"
#include "glm/glm.hpp"

#include <algorithm>
#include <cstring>
#include <queue>

#include "core/engine/subsystem/subsystem_registry.h"
#include "default_engine_icon_paths.h"

// --- Colour palette ------------------------------------------------------------
static constexpr ImU32 COL_PANEL_BG          = IM_COL32(30,  30,  30,  255);
static constexpr ImU32 COL_TOOLBAR_BG        = IM_COL32(38,  38,  38,  255);

static constexpr ImU32 COL_TILE_BG           = IM_COL32(36,  36,  36,  255);
static constexpr ImU32 COL_TILE_BG_HOVER     = IM_COL32(52,  52,  52,  255);
static constexpr ImU32 COL_TILE_BORDER       = IM_COL32(60,  60,  60,  255);
static constexpr ImU32 COL_TILE_BORDER_HOVER = IM_COL32(110, 110, 110, 255);
static constexpr ImU32 COL_TILE_SELECTED     = IM_COL32(26,  95,  180, 255);
static constexpr ImU32 COL_TILE_BORDER_SEL   = IM_COL32(70,  150, 255, 255);
static constexpr ImU32 COL_TILE_SHADOW       = IM_COL32(0,   0,   0,   80);

static constexpr ImU32 COL_TYPE_MESH         = IM_COL32(0,  140, 200, 255);
static constexpr ImU32 COL_TYPE_MATERIAL     = IM_COL32(30, 160,  90, 255);
static constexpr ImU32 COL_TYPE_TEXTURE      = IM_COL32(180, 80, 180, 255);
static constexpr ImU32 COL_TYPE_SHADER       = IM_COL32(220,130,  30, 255);
static constexpr ImU32 COL_TYPE_SCENE        = IM_COL32(200, 60,  60, 255);
static constexpr ImU32 COL_TYPE_AUDIO        = IM_COL32(60, 180, 180, 255);
static constexpr ImU32 COL_TYPE_FOLDER       = IM_COL32(210,155,  40, 255);
static constexpr ImU32 COL_TYPE_DEFAULT      = IM_COL32(90,  90,  90, 255);

static constexpr ImU32 COL_ROW_EVEN          = IM_COL32(36,  36,  36,  255);
static constexpr ImU32 COL_ROW_ODD           = IM_COL32(42,  42,  42,  255);
static constexpr ImU32 COL_ROW_HOVER         = IM_COL32(255, 255, 255, 18);
static constexpr ImU32 COL_ROW_SELECTED      = IM_COL32(26,  95,  180, 200);

static constexpr ImU32 COL_TEXT_DIM          = IM_COL32(140, 140, 140, 255);
static constexpr ImU32 COL_SEPARATOR         = IM_COL32(55,  55,  55,  255);
static constexpr ImU32 COL_TREE_SELECTED     = IM_COL32(26,  95,  180, 200);
static constexpr ImU32 COL_RELOAD_FLASH      = IM_COL32(80,  200, 120, 255);

static constexpr float TILE_ROUNDING  = 6.0f;
static constexpr float TYPE_BAR_H     = 16.0f;
static constexpr float NAME_AREA_H    = 30.0f;
static constexpr float THUMB_PAD      = 3.0f;
static constexpr float CARD_HPAD      = 5.0f;
static constexpr float SHADOW_OFFSET  = 3.0f;

// --- Helpers -----------------------------------------------------------------

static bool caseInsensitiveContains(const std::string& haystack,
                                    const std::string& needle)
{
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(),   needle.end(),
        [](char a, char b){ return std::tolower(a) == std::tolower(b); });
    return it != haystack.end();
}

// --- Constructor / Destructor ------------------------------------------------

MEditorAssetWindow::MEditorAssetWindow() : MEditorAssetWindow(700, 300) {}

MEditorAssetWindow::MEditorAssetWindow(int x, int y) : MImGuiSubWindow(x, y)
{
    title = "Asset Browser";

    auto* editorAssetManager = dynamic_cast<MEditorAssetManager*>(MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
    rootNode = editorAssetManager ? editorAssetManager->getAssetRootNode() : nullptr;

    if (editorAssetManager)
        lastSeenReloadCount = editorAssetManager->getTotalHotReloadCount();

    if (!rootNode) return;

    for (const auto& child : rootNode->getChildrenNodes())
    {
        auto name = child->getName();
        name.toLowerCase();
        if (name == "assets")
        {
            navigateTo(child);
            return;
        }
    }
    navigateTo(rootNode);
}

MEditorAssetWindow::~MEditorAssetWindow() {}

// --- Hot reload sync ----------------------------------------------------------

void MEditorAssetWindow::tickAndSync(float deltaTime)
{
    auto* editorAM = dynamic_cast<MEditorAssetManager*>(MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
    if (!editorAM) return;

    if (rootNode != editorAM->getAssetRootNode())
        syncWithAssetManager();

    const int currentCount = editorAM->getTotalHotReloadCount();
    if (currentCount != lastSeenReloadCount)
    {
        lastSeenReloadCount = currentCount;
        reloadFlashTimer    = RELOAD_FLASH_DURATION;
    }

    reloadFlashTimer = std::max(0.0f, reloadFlashTimer - deltaTime);

    processPendingPing();
}

void MEditorAssetWindow::syncWithAssetManager()
{
    auto* editorAM = dynamic_cast<MEditorAssetManager*>(MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
    if (!editorAM) return;

    const SString savedCurrentPath  = cachedCurrentPath;
    const SString savedSelectedPath = cachedSelectedPath;
    std::vector<SString> savedHistoryBack    = cachedHistoryBackPaths;
    std::vector<SString> savedHistoryForward = cachedHistoryForwardPaths;

    rootNode             = nullptr;
    currentDirectoryNode = nullptr;
    selectedNode         = nullptr;
    rightClickedNode     = nullptr;
    pendingDeleteNode    = nullptr;
    historyBack.clear();
    historyForward.clear();
    cachedCurrentPath.clear();
    cachedSelectedPath.clear();
    cachedHistoryBackPaths.clear();
    cachedHistoryForwardPaths.clear();

    rootNode = editorAM->getAssetRootNode();
    if (!rootNode) return;

    currentDirectoryNode = findNodeByPath(rootNode, savedCurrentPath);
    if (!currentDirectoryNode)
        currentDirectoryNode = rootNode;
    cachedCurrentPath = currentDirectoryNode->getPath();

    for (const auto& path : savedHistoryBack)
    {
        if (path.empty()) continue;
        if (auto* node = findNodeByPath(rootNode, path))
            if (node->isDirectory)
            {
                historyBack.push_back(node);
                cachedHistoryBackPaths.push_back(path);
            }
    }
    for (const auto& path : savedHistoryForward)
    {
        if (path.empty()) continue;
        if (auto* node = findNodeByPath(rootNode, path))
            if (node->isDirectory)
            {
                historyForward.push_back(node);
                cachedHistoryForwardPaths.push_back(path);
            }
    }

    if (!savedSelectedPath.empty())
    {
        selectedNode = findNodeByPath(rootNode, savedSelectedPath);
        cachedSelectedPath = selectedNode ? savedSelectedPath : SString();
    }
}

// --- processPendingPing ------------------------------------------------------

void MEditorAssetWindow::processPendingPing()
{
    auto* editorAM = dynamic_cast<MEditorAssetManager*>(MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
    if (!editorAM || !rootNode) return;

    const SString assetId = editorAM->consumePendingPing();
    if (assetId.empty()) return;

    SAssetDirectoryNode* parent = nullptr;
    SAssetDirectoryNode* target = findNodeByAssetId(rootNode, assetId, &parent);
    if (!target) return;

    if (parent && parent != currentDirectoryNode)
        navigateTo(parent);

    selectedNode = target;
    cachedSelectedPath = target->getPath();
    selectAssetForInspector(target);
}

SAssetDirectoryNode* MEditorAssetWindow::findNodeByAssetId(SAssetDirectoryNode* root,
                                                            const SString& assetId,
                                                            SAssetDirectoryNode** outParent)
{
    if (!root) return nullptr;

    std::queue<std::pair<SAssetDirectoryNode*, SAssetDirectoryNode*>> q;
    q.push({root, nullptr});

    while (!q.empty())
    {
        auto [node, parent] = q.front();
        q.pop();

        if (!node->isDirectory &&
            node->assetReference &&
            node->assetReference->getAssetId() == assetId)
        {
            if (outParent) *outParent = parent;
            return node;
        }

        for (auto* child : node->getChildrenNodes())
            if (child) q.push({child, node});
    }
    return nullptr;
}

// --- findNodeByPath -----------------------------------------------------------

SAssetDirectoryNode* MEditorAssetWindow::findNodeByPath(SAssetDirectoryNode* root,
                                                         const SString& path)
{
    if (!root) return nullptr;

    std::queue<SAssetDirectoryNode*> q;
    q.push(root);
    while (!q.empty())
    {
        auto* node = q.front(); q.pop();
        if (node->getPath() == path) return node;
        for (auto* child : node->getChildrenNodes())
            if (child) q.push(child);
    }
    return nullptr;
}

// --- Top-level render --------------------------------------------------------

void MEditorAssetWindow::onGui(float deltaTime)
{
    tickAndSync(deltaTime);

    if (!currentDirectoryNode) return;

    drawToolbar();

    float availHeight = ImGui::GetContentRegionAvail().y - 24.0f;

    if (showSources)
    {
        ImGui::BeginChild("##sources", ImVec2(sourcesWidth, availHeight), false);
        drawSourcesPanel();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f,0.6f,1.0f,0.4f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.4f,0.6f,1.0f,0.6f));
        ImGui::Button("##splitter", ImVec2(4.0f, availHeight));
        ImGui::PopStyleColor(3);

        if (ImGui::IsItemActive())
        {
            sourcesWidth += ImGui::GetIO().MouseDelta.x;
            sourcesWidth = std::clamp(sourcesWidth, 80.0f, 500.0f);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        ImGui::SameLine();
    }

    ImGui::BeginChild("##content", ImVec2(0, availHeight), false);
    drawBreadcrumbs();
    drawContentArea();
    ImGui::EndChild();

    // Delete confirmation modal (drawn at window scope so it isn't clipped).
    drawDeleteConfirmModal();

    // -- Status bar ------------------------------------------------------------
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 statusPos = ImGui::GetWindowPos();
    ImVec2 winSize   = ImGui::GetWindowSize();
    statusPos.y += winSize.y - 22.0f;
    dl->AddRectFilled(statusPos,
                      ImVec2(statusPos.x + winSize.x, statusPos.y + 22.0f),
                      COL_TOOLBAR_BG);

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 20.0f);
    ImGui::SetCursorPosX(8.0f);

    int totalItems = 0;
    for (const auto& n : currentDirectoryNode->getChildrenNodes())
        if (matchesSearch(n)) totalItems++;

    const char* selectedName = selectedNode ? selectedNode->getName().c_str() : "";
    if (selectedNode)
        ImGui::Text("%d items  |  Selected: %s", totalItems, selectedName);
    else
        ImGui::Text("%d items", totalItems);

    if (reloadFlashTimer > 0.0f)
    {
        const float alpha = reloadFlashTimer / RELOAD_FLASH_DURATION;
        ImU32 flashCol = IM_COL32(80, 200, 120, (int)(alpha * 255));

        ImGui::SameLine(0, 20);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(flashCol));
        ImGui::Text("  Hot Reloaded  %d", lastSeenReloadCount);
        ImGui::PopStyleColor();
    }
}

// --- Toolbar -----------------------------------------------------------------

void MEditorAssetWindow::drawToolbar()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(COL_TOOLBAR_BG));
    ImGui::BeginChild("##toolbar", ImVec2(0, 32), false);

    ImGui::SetCursorPosY(6.0f);

    ImGui::BeginDisabled(historyBack.size() <= 1);
    if (ImGui::Button(" < ")) navigateBack();
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(historyForward.empty());
    if (ImGui::Button(" > "))
    {
        auto* next = historyForward.back();
        SString nextPath = cachedHistoryForwardPaths.back();
        historyForward.pop_back();
        cachedHistoryForwardPaths.pop_back();

        historyBack.push_back(currentDirectoryNode);
        cachedHistoryBackPaths.push_back(cachedCurrentPath);

        currentDirectoryNode = next;
        cachedCurrentPath    = nextPath;
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0, 12);

    ImGui::SetNextItemWidth(220);
    ImGui::InputTextWithHint("##search", "  Search assets...", searchBuffer, sizeof(searchBuffer));

    ImGui::SameLine(0, 12);

    ImGui::SetNextItemWidth(110);
    const char* sortLabels[] = { "Name", "Type", "Date" };
    int sortIdx = (int)sortMode;
    if (ImGui::Combo("##sort", &sortIdx, sortLabels, 3))
        sortMode = (EAssetSortMode)sortIdx;

    ImGui::SameLine(0, 12);

    if (ImGui::Button(viewMode == EAssetViewMode::Grid ? "[#]" : "[=]"))
        viewMode = (viewMode == EAssetViewMode::Grid) ? EAssetViewMode::List
                                                       : EAssetViewMode::Grid;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle Grid / List view");

    ImGui::SameLine(0, 6);

    if (ImGui::Button(showSources ? "[ | ]" : "[ ]"))
        showSources = !showSources;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle Sources panel");

    ImGui::SameLine(0, 12);

    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##zoom", &zoomLevel, 0.5f, 2.0f, "Zoom %.1f");

    if (reloadFlashTimer > 0.0f)
    {
        const float alpha = reloadFlashTimer / RELOAD_FLASH_DURATION;
        ImU32 col = IM_COL32(80, 200, 120, (int)(alpha * 220));

        ImGui::SameLine(0, 16);
        ImVec2 dotPos = ImGui::GetCursorScreenPos();
        dotPos.y += 8.0f;
        ImGui::GetWindowDrawList()->AddCircleFilled(dotPos, 5.0f, col);
        ImGui::Dummy(ImVec2(12, 0));

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Assets hot-reloaded");
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// --- Left panel – folder tree ------------------------------------------------

void MEditorAssetWindow::drawSourcesPanel()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(COL_PANEL_BG));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    ImGui::Text("  SOURCES");
    ImGui::Separator();

    if (rootNode)
        drawDirectoryTree(rootNode);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void MEditorAssetWindow::drawDirectoryTree(SAssetDirectoryNode* node, int depth)
{
    if (!node) return;
    if (!node->isDirectory) return;

    bool isCurrentDir = (node == currentDirectoryNode);
    bool hasChildren  = false;
    for (const auto& child : node->getChildrenNodes())
        if (child && child->isDirectory) { hasChildren = true; break; }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth
                             | ImGuiTreeNodeFlags_OpenOnArrow;
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;
    if (isCurrentDir) flags |= ImGuiTreeNodeFlags_Selected;

    if (isCurrentDir)
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float  w = ImGui::GetContentRegionAvail().x;
        ImGui::GetWindowDrawList()->AddRectFilled(
            p, ImVec2(p.x + w, p.y + ImGui::GetTextLineHeightWithSpacing()),
            COL_TREE_SELECTED);
    }

    bool open = ImGui::TreeNodeEx(node->getName().c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        navigateTo(node);

    if (open)
    {
        for (const auto& child : node->getChildrenNodes())
            drawDirectoryTree(child, depth + 1);
        ImGui::TreePop();
    }
}

// --- Breadcrumbs -------------------------------------------------------------

void MEditorAssetWindow::drawBreadcrumbs()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(COL_TOOLBAR_BG));
    ImGui::BeginChild("##breadcrumbs", ImVec2(0, 26), false);
    ImGui::SetCursorPosY(5.0f);

    if (!rootNode || !currentDirectoryNode)
    {
        ImGui::EndChild();
        ImGui::PopStyleColor();
        return;
    }

    const bool atRoot = (currentDirectoryNode == rootNode);
    if (atRoot)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        ImGui::Text("%s", rootNode->getName().c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(COL_TEXT_DIM));
        if (ImGui::SmallButton(rootNode->getName().c_str()))
            navigateTo(rootNode);
        ImGui::PopStyleColor();

        auto segments = currentDirectoryNode->getPath().split("/");
        SAssetDirectoryNode* walker = rootNode;

        for (int i = 0; i < (int)segments.size(); ++i)
        {
            if (!walker) break;

            auto* child = walker->getChild(segments[i]);
            if (!child || !child->isDirectory) break;
            walker = child;

            ImGui::SameLine(0, 2);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(COL_TEXT_DIM));
            ImGui::TextUnformatted("/");
            ImGui::PopStyleColor();
            ImGui::SameLine(0, 2);

            ImGui::PushID(i);
            const bool isLast = (i == (int)segments.size() - 1);
            if (isLast)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                ImGui::Text("%s", segments[i].c_str());
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(COL_TEXT_DIM));
                if (ImGui::SmallButton(segments[i].c_str()))
                    navigateTo(walker);
                ImGui::PopStyleColor();
            }
            ImGui::PopID();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// --- Content area dispatcher -------------------------------------------------

void MEditorAssetWindow::drawContentArea()
{
    if (viewMode == EAssetViewMode::Grid)
        drawAssetGrid(currentDirectoryNode);
    else
        drawAssetList(currentDirectoryNode);

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !pendingContextMenu)
    {
        ImGui::OpenPopup("##ctx_empty");
    }
    if (ImGui::BeginPopup("##ctx_empty"))
    {
        ImGui::TextDisabled("This Folder");
        ImGui::Separator();

        // Create submenu — populated automatically by registered MMenubarItems
        // (Material, Shader, Skybox, etc. under Assets/Create/Rendering/...).
        auto* createNode = MMenubarTreeNode::getNodeAtPath("Assets/Create");
        if (createNode)
        {
            if (ImGui::BeginMenu("Create"))
            {
                createNode->renderAsContextMenu();
                ImGui::EndMenu();
            }
            ImGui::Separator();
        }

        if (ImGui::MenuItem("Show in File Explorer"))
        {
            auto cmd = STR("\"") + currentDirectoryNode->getPath() + STR("\"");
            system(cmd.c_str());
        }
        ImGui::EndPopup();
    }
}

// --- Helpers: get sorted + filtered children ---------------------------------

static std::vector<SAssetDirectoryNode*> getSortedChildren(
    SAssetDirectoryNode* dir,
    EAssetSortMode sort,
    bool filterDirs,
    bool filterFiles,
    const char* search)
{
    std::vector<SAssetDirectoryNode*> dirs, files;

    for (const auto& child : dir->getChildrenNodes())
    {
        if (!child) continue;
        std::string name = child->getName();
        if (strlen(search) > 0)
            if (!caseInsensitiveContains(name, search)) continue;
        if (child->isDirectory) { if (filterDirs)  dirs.push_back(child);  }
        else                    { if (filterFiles) files.push_back(child); }
    }

    auto byName = [](SAssetDirectoryNode* a, SAssetDirectoryNode* b){
        return a->getName() < b->getName();
    };
    std::sort(dirs.begin(),  dirs.end(),  byName);
    std::sort(files.begin(), files.end(), byName);

    std::vector<SAssetDirectoryNode*> result;
    result.insert(result.end(), dirs.begin(),  dirs.end());
    result.insert(result.end(), files.begin(), files.end());
    return result;
}

static ImU32 getTileTypeColor(SAssetDirectoryNode* node)
{
    if (!node) return COL_TYPE_DEFAULT;
    if (node->isDirectory) return COL_TYPE_FOLDER;

    const std::string path = node->getPath().str();
    auto ext = path.substr(path.find_last_of('.') + 1);
    for (auto& c : ext) c = (char)std::tolower(c);

    if (ext == "obj" || ext == "fbx" || ext == "gltf" || ext == "glb" ||
        ext == "dae" || ext == "3ds" || ext == "stl")
        return COL_TYPE_MESH;
    if (ext == "mat" || ext == "material")
        return COL_TYPE_MATERIAL;
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
        ext == "tga" || ext == "hdr" || ext == "exr" || ext == "dds")
        return COL_TYPE_TEXTURE;
    if (ext == "mesl" || ext == "glsl" || ext == "vert" || ext == "frag" ||
        ext == "cubemap")
        return COL_TYPE_SHADER;
    if (ext == "mscene" || ext == "scene")
        return COL_TYPE_SCENE;
    if (ext == "wav" || ext == "ogg" || ext == "mp3" || ext == "flac")
        return COL_TYPE_AUDIO;

    return COL_TYPE_DEFAULT;
}

static std::string getTileTypeLabel(SAssetDirectoryNode* node)
{
    if (!node || node->isDirectory) return "Folder";

    const std::string path = node->getPath().str();
    const auto dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) return "Asset";

    std::string ext = path.substr(dotPos + 1);
    for (auto& c : ext) c = (char)std::tolower(c);

    if (ext == "obj" || ext == "fbx" || ext == "gltf" || ext == "glb" ||
        ext == "dae" || ext == "3ds" || ext == "stl")  return "Static Mesh";
    if (ext == "mat" || ext == "material")              return "Material";
    if (ext == "png" || ext == "jpg" || ext == "jpeg" ||
        ext == "bmp" || ext == "tga" || ext == "hdr" ||
        ext == "exr" || ext == "dds")                   return "Texture";
    if (ext == "mesl")                                  return "Shader";
    if (ext == "glsl" || ext == "vert" || ext == "frag")return "GLSL";
    if (ext == "cubemap")                               return "Skybox";
    if (ext == "mscene" || ext == "scene")              return "Scene";
    if (ext == "wav" || ext == "ogg" || ext == "mp3")   return "Audio";
    if (ext == "txt" || ext == "json" || ext == "xml" ||
        ext == "yaml" || ext == "csv")                  return "Data";

    return ext.empty() ? "Asset" : ext;
}

bool MEditorAssetWindow::matchesSearch(SAssetDirectoryNode* node) const
{
    if (strlen(searchBuffer) == 0) return true;
    return caseInsensitiveContains(node->getName(), searchBuffer);
}

// --- Grid view ---------------------------------------------------------------

void MEditorAssetWindow::drawAssetGrid(SAssetDirectoryNode* root)
{
    if (!root) return;

    auto* am  = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    float iconSize     = 64.0f * zoomLevel;
    float cellSize     = iconSize + CARD_HPAD * 2.0f + 6.0f;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int   colCount   = std::max(1, (int)(panelWidth / cellSize));

    ImGui::Columns(colCount, nullptr, false);

    auto children = getSortedChildren(root, sortMode, filterDirectories, filterFiles, searchBuffer);

    for (auto* node : children)
    {
        ImGui::PushID(node);
        drawAssetTile(node, am, iconSize);
        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) &&
        !ImGui::IsAnyItemHovered())
    {
        selectedNode = nullptr;
        cachedSelectedPath.clear();
    }
}

void MEditorAssetWindow::drawAssetTile(SAssetDirectoryNode* node,
                                       IAssetManagerSubsystem* am,
                                       float iconSize)
{
    const bool isSelected = (node == selectedNode);

    const float thumbSz = iconSize;
    const float cardW   = thumbSz + CARD_HPAD * 2.0f;
    const float cardH   = thumbSz + NAME_AREA_H + TYPE_BAR_H + THUMB_PAD * 2.0f;

    sf::Texture* icon        = nullptr;
    bool         isThumbnail = false;

    if (node->isDirectory)
    {
        icon = am->getAsset<MTextureAsset>(SEditorAssetPaths::HIGHRES_TEX_ASSET_FOLDER)
                  ->getTexture()->getCoreTexture();
    }
    else if (node->assetReference)
    {
        auto* editorAM = dynamic_cast<MEditorAssetManager*>(am);
        if (editorAM)
        {
            sf::Texture* thumb = editorAM->getThumbnail(node->assetReference);
            if (thumb) { icon = thumb; isThumbnail = true; }
            else       { editorAM->requestThumbnail(node->assetReference); icon = getFileIcon(am, node); }
        }
        else { icon = getFileIcon(am, node); }
    }

    ImGui::BeginGroup();
    ImVec2 p = ImGui::GetCursorScreenPos();

    bool clicked  = ImGui::InvisibleButton("##tile", ImVec2(cardW, cardH));
    bool hovered  = ImGui::IsItemHovered();
    bool dblClick = hovered && ImGui::IsMouseDoubleClicked(0);

    if (!node->isDirectory && icon)
        doAssetDragSource(MAssetReferenceControl::ASSET_REF_TARGET_KEY.c_str(), *icon, node);

    if (clicked)  { selectedNode = node; cachedSelectedPath = node->getPath(); selectAssetForInspector(node); }
    if (dblClick) { if (node->isDirectory) navigateTo(node); else onFileDoubleClicked(node); }
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        rightClickedNode   = node;
        pendingContextMenu = true;
        ImGui::OpenPopup("##assetctx");
    }
    openContextMenu(node);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    const ImVec2 br       (p.x + cardW,         p.y + cardH);
    const ImVec2 thumbTL  (p.x + CARD_HPAD,     p.y + THUMB_PAD);
    const ImVec2 thumbBR  (p.x + CARD_HPAD + thumbSz, p.y + THUMB_PAD + thumbSz);
    const ImVec2 nameTL   (p.x,                 thumbBR.y);
    const ImVec2 nameBR   (br.x,                nameTL.y + NAME_AREA_H);
    const ImVec2 typeBarTL(p.x,                 nameBR.y);
    const ImU32  typeCol  = getTileTypeColor(node);

    dl->AddRectFilled(
        ImVec2(p.x + SHADOW_OFFSET, p.y + SHADOW_OFFSET),
        ImVec2(br.x + SHADOW_OFFSET, br.y + SHADOW_OFFSET),
        COL_TILE_SHADOW, TILE_ROUNDING);

    const ImU32 bgCol = isSelected ? COL_TILE_SELECTED
                      : hovered    ? COL_TILE_BG_HOVER
                                   : COL_TILE_BG;
    dl->AddRectFilled(p, br, bgCol, TILE_ROUNDING);

    if (icon)
    {
        dl->PushClipRect(thumbTL, thumbBR, true);

        if (isThumbnail)
        {
            dl->AddImage(
                (ImTextureID)(intptr_t)icon->getNativeHandle(),
                thumbTL, thumbBR,
                ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32(255, 255, 255, 255));
        }
        else
        {
            dl->AddRectFilled(thumbTL, thumbBR, IM_COL32(28, 28, 28, 220));
            const float sz = thumbSz * 0.60f;
            const ImVec2 ic(thumbTL.x + (thumbSz - sz) * 0.5f,
                            thumbTL.y + (thumbSz - sz) * 0.5f);
            dl->AddImage(
                (ImTextureID)(intptr_t)icon->getNativeHandle(),
                ic, ImVec2(ic.x + sz, ic.y + sz),
                ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32(255, 255, 255, 210));
        }

        dl->PopClipRect();
    }
    else
    {
        dl->AddRectFilled(thumbTL, thumbBR, IM_COL32(28, 28, 28, 220));
    }

    {
        const float  maxNameW  = cardW - CARD_HPAD * 2.0f;
        const float  lineH     = ImGui::GetTextLineHeight();
        const float  textY     = nameTL.y + (NAME_AREA_H - lineH) * 0.5f;

        const std::string fullName = node->getName();
        const char*       nameC    = fullName.c_str();
        std::string       truncated;

        if (ImGui::CalcTextSize(nameC).x > maxNameW)
        {
            int lo = 0, hi = (int)fullName.size();
            while (lo < hi)
            {
                int mid = (lo + hi + 1) / 2;
                std::string candidate = fullName.substr(0, mid) + "...";
                if (ImGui::CalcTextSize(candidate.c_str()).x <= maxNameW) lo = mid;
                else hi = mid - 1;
            }
            truncated = fullName.substr(0, lo) + "...";
            nameC     = truncated.c_str();
        }

        const float textW = ImGui::CalcTextSize(nameC).x;
        const float textX = p.x + CARD_HPAD + (maxNameW - textW) * 0.5f;

        dl->AddText(ImVec2(textX, textY),
                    isSelected ? IM_COL32(255, 255, 255, 255)
                               : IM_COL32(210, 210, 210, 255),
                    nameC);
    }

    dl->AddRectFilled(typeBarTL, br, typeCol,
                      TILE_ROUNDING, ImDrawFlags_RoundCornersBottom);

    {
        std::string  labelStr  = getTileTypeLabel(node);
        const float  maxLabelW = cardW - CARD_HPAD * 2.0f;

        if (ImGui::CalcTextSize(labelStr.c_str()).x > maxLabelW)
        {
            int lo = 0, hi = (int)labelStr.size();
            while (lo < hi)
            {
                int mid = (lo + hi + 1) / 2;
                std::string cand = labelStr.substr(0, mid) + "..";
                if (ImGui::CalcTextSize(cand.c_str()).x <= maxLabelW) lo = mid;
                else hi = mid - 1;
            }
            labelStr = labelStr.substr(0, lo) + "..";
        }

        const float  lw    = ImGui::CalcTextSize(labelStr.c_str()).x;
        const float  lx    = p.x + CARD_HPAD + (maxLabelW - lw) * 0.5f;
        const float  ly    = typeBarTL.y + (TYPE_BAR_H - ImGui::GetTextLineHeight()) * 0.5f;
        dl->AddText(ImVec2(lx, ly), IM_COL32(255, 255, 255, 230), labelStr.c_str());
    }

    const ImU32  borderCol = isSelected ? COL_TILE_BORDER_SEL
                           : hovered    ? COL_TILE_BORDER_HOVER
                                        : COL_TILE_BORDER;
    const float  borderW   = isSelected ? 2.0f : 1.0f;
    dl->AddRect(p, br, borderCol, TILE_ROUNDING, 0, borderW);

    if (hovered)
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(node->getName().c_str());
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(COL_TEXT_DIM));
        ImGui::TextUnformatted(node->getPath().c_str());
        ImGui::PopStyleColor();
        ImGui::EndTooltip();
    }

    ImGui::EndGroup();
}


// --- List view ---------------------------------------------------------------

void MEditorAssetWindow::drawAssetList(SAssetDirectoryNode* root)
{
    if (!root) return;

    auto* am = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();

    ImGui::Columns(3, "##listcols", false);
    ImGui::SetColumnWidth(0, 300);
    ImGui::SetColumnWidth(1, 120);

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(COL_TEXT_DIM));
    ImGui::Text("  Name");  ImGui::NextColumn();
    ImGui::Text("Type");    ImGui::NextColumn();
    ImGui::Text("Path");    ImGui::NextColumn();
    ImGui::PopStyleColor();

    ImVec2 sep0 = ImGui::GetCursorScreenPos();
    sep0.y -= 2;
    ImGui::GetWindowDrawList()->AddLine(
        sep0, ImVec2(sep0.x + ImGui::GetContentRegionAvail().x, sep0.y), COL_SEPARATOR);

    ImGui::Columns(1);

    auto children = getSortedChildren(root, sortMode, filterDirectories, filterFiles, searchBuffer);

    for (int i = 0; i < (int)children.size(); ++i)
    {
        ImGui::PushID(children[i]);
        drawAssetListRow(children[i], am, i);
        ImGui::PopID();
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) &&
        !ImGui::IsAnyItemHovered())
    {
        selectedNode = nullptr;
        cachedSelectedPath.clear();
    }
}

void MEditorAssetWindow::drawAssetListRow(SAssetDirectoryNode* node,
                                          IAssetManagerSubsystem* am,
                                          int rowIndex)
{
    bool isSelected = (node == selectedNode);
    float rowH = 22.0f;
    float rowW = ImGui::GetContentRegionAvail().x;

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImU32 bg = isSelected ? COL_ROW_SELECTED
             : (rowIndex % 2 == 0) ? COL_ROW_EVEN : COL_ROW_ODD;
    dl->AddRectFilled(p, ImVec2(p.x + rowW, p.y + rowH), bg);

    sf::Texture* icon = nullptr;
    if (node->isDirectory)
    {
        icon = am->getAsset<MTextureAsset>(SEditorAssetPaths::HIGHRES_TEX_ASSET_FOLDER)
                  ->getTexture()->getCoreTexture();
    }
    else if (node->assetReference)
    {
        auto* editorAM = dynamic_cast<MEditorAssetManager*>(am);
        if (editorAM)
        {
            sf::Texture* thumb = editorAM->getThumbnail(node->assetReference);
            icon = thumb ? thumb : getFileIcon(am, node);
            if (!thumb) editorAM->requestThumbnail(node->assetReference);
        }
        else icon = getFileIcon(am, node);
    }

    bool clicked  = ImGui::InvisibleButton("##row", ImVec2(rowW, rowH));
    bool hovered  = ImGui::IsItemHovered();
    bool dblClick = hovered && ImGui::IsMouseDoubleClicked(0);

    if (!node->isDirectory && icon)
        doAssetDragSource(MAssetReferenceControl::ASSET_REF_TARGET_KEY.c_str(), *icon, node);

    if (hovered && !isSelected)
        dl->AddRectFilled(p, ImVec2(p.x + rowW, p.y + rowH), COL_ROW_HOVER);

    if (clicked)  { selectedNode = node; cachedSelectedPath = node->getPath(); selectAssetForInspector(node); }
    if (dblClick)
    {
        if (node->isDirectory) navigateTo(node);
        else                   onFileDoubleClicked(node);
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        rightClickedNode   = node;
        pendingContextMenu = true;
        ImGui::OpenPopup("##assetctx");
    }
    openContextMenu(node);

    ImGui::SetCursorScreenPos(ImVec2(p.x + 4,  p.y + 3));
    if (icon) ImGui::Image(*icon, ImVec2(16, 16));
    ImGui::SetCursorScreenPos(ImVec2(p.x + 24, p.y + 3));
    ImGui::TextUnformatted(node->getName().c_str());

    ImGui::SetCursorScreenPos(ImVec2(p.x + 304, p.y + 3));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(COL_TEXT_DIM));
    ImGui::TextUnformatted(getTileTypeLabel(node).c_str());

    ImGui::SetCursorScreenPos(ImVec2(p.x + 428, p.y + 3));
    ImGui::TextUnformatted(node->getPath().c_str());
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + rowH + 1));
}

// --- Context menu -------------------------------------------------------------

void MEditorAssetWindow::openContextMenu(SAssetDirectoryNode* node)
{
    if (!ImGui::BeginPopup("##assetctx")) return;

    pendingContextMenu = false;
    auto* target = rightClickedNode ? rightClickedNode : node;

    ImGui::TextDisabled("%s", target ? target->getName().c_str() : "Asset");
    ImGui::Separator();

    if (target && target->isDirectory)
    {
        if (ImGui::MenuItem("Open Folder")) navigateTo(target);
        ImGui::Separator();
    }
    if (target && !target->isDirectory)
    {
        if (ImGui::MenuItem("Open")) onFileDoubleClicked(target);
        ImGui::Separator();
    }

    // Create submenu — populated by registered MMenubarItems.
    auto* createNode = MMenubarTreeNode::getNodeAtPath("Assets/Create");
    if (createNode)
    {
        if (ImGui::BeginMenu("Create"))
        {
            createNode->renderAsContextMenu();
            ImGui::EndMenu();
        }
        ImGui::Separator();
    }

    // -- Delete ----------------------------------------------------------------
    if (target && !target->isDirectory && target->assetReference)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
        if (ImGui::MenuItem("Delete"))
        {
            pendingDeleteNode    = target;
            pendingDeleteConfirm = true;
        }
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    if (ImGui::MenuItem("Show in File Explorer"))
    {
        auto path = target ? target->getPath() : currentDirectoryNode->getPath();
        auto cmd  = STR("\"") + path + STR("\"");
        system(cmd.c_str());
    }
    if (ImGui::MenuItem("Copy Path"))
    {
        auto path = target ? target->getPath() : currentDirectoryNode->getPath();
        ImGui::SetClipboardText(path.c_str());
    }

    ImGui::EndPopup();
}

// --- Delete confirmation modal -----------------------------------------------

void MEditorAssetWindow::drawDeleteConfirmModal()
{
    if (pendingDeleteConfirm)
    {
        ImGui::OpenPopup("Delete Asset?");
        pendingDeleteConfirm = false;
    }

    if (ImGui::BeginPopupModal("Delete Asset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const char* name = pendingDeleteNode ? pendingDeleteNode->getName().c_str() : "?";
        ImGui::Text("Are you sure you want to delete \"%s\"?", name);
        ImGui::Text("This cannot be undone.");
        ImGui::Spacing();

        if (ImGui::Button("Delete", ImVec2(120, 0)))
        {
            if (pendingDeleteNode && pendingDeleteNode->assetReference)
            {
                auto* editorAM = dynamic_cast<MEditorAssetManager*>(
                    MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
                if (editorAM)
                    editorAM->deleteAsset(pendingDeleteNode->assetReference);
            }

            if (selectedNode == pendingDeleteNode)
            {
                selectedNode = nullptr;
                cachedSelectedPath.clear();
            }
            pendingDeleteNode = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            pendingDeleteNode = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}


// --- Navigation --------------------------------------------------------------

void MEditorAssetWindow::navigateTo(SAssetDirectoryNode* node)
{
    if (!node || node == currentDirectoryNode) return;

    historyForward.clear();
    cachedHistoryForwardPaths.clear();

    if (currentDirectoryNode)
    {
        historyBack.push_back(currentDirectoryNode);
        cachedHistoryBackPaths.push_back(cachedCurrentPath);
    }

    currentDirectoryNode = node;
    cachedCurrentPath    = node->getPath();
    selectedNode         = nullptr;
    cachedSelectedPath.clear();
}

void MEditorAssetWindow::navigateBack()
{
    if (historyBack.size() <= 1) return;

    historyForward.push_back(currentDirectoryNode);
    cachedHistoryForwardPaths.push_back(cachedCurrentPath);

    currentDirectoryNode = historyBack.back();
    cachedCurrentPath    = cachedHistoryBackPaths.back();
    historyBack.pop_back();
    cachedHistoryBackPaths.pop_back();

    selectedNode = nullptr;
    cachedSelectedPath.clear();
}

// --- Helpers -----------------------------------------------------------------

void MEditorAssetWindow::drawTruncatedLabel(const std::string& text, float maxWidth)
{
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + maxWidth);
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopTextWrapPos();
}

void MEditorAssetWindow::selectAssetForInspector(SAssetDirectoryNode* node)
{
    if (!node || node->isDirectory || !node->assetReference) return;
    MEditorApplication::SelectedObject = node->assetReference;
}

void MEditorAssetWindow::onFileDoubleClicked(SAssetDirectoryNode* node)
{
    if (!node || node->isDirectory || !node->assetReference)
        return;
    auto* editorAssetManager = dynamic_cast<MEditorAssetManager*>(MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
    if (editorAssetManager)
        editorAssetManager->openAsset(node->assetReference);
}



sf::Texture* MEditorAssetWindow::getFileIcon(IAssetManagerSubsystem* am, SAssetDirectoryNode* asset) const
{
    auto* texAsset = dynamic_cast<MTextureAsset*>(asset->getAsset());
    if (texAsset && texAsset->getTexture()->getCoreTexture())
        return texAsset->getTexture()->getCoreTexture();

    for (const auto& importer : *MAssetImporter::getImporters())
    {
        auto ext = FileIO::getFileExtension(asset->getPath());
        if (importer->canImport(ext))
            return am->getAsset<MTextureAsset>(importer->getIconPath())
                      ->getTexture()->getCoreTexture();
    }

    return am->getAsset<MTextureAsset>(
               SEditorAssetPaths::HIGHRES_TEX_ASSET_DEFAULT)
               ->getTexture()->getCoreTexture();
}

void MEditorAssetWindow::doAssetDragSource(SString key,
                                            const sf::Texture& icon,
                                            SAssetDirectoryNode* node)
{
    if (!node->assetReference) return;

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        draggedAssetId = node->assetReference->getAssetId();
        char payloadBuf[512] = {};
        strncpy(payloadBuf, draggedAssetId.c_str(), sizeof(payloadBuf) - 1);
        ImGui::SetDragDropPayload(key.c_str(), payloadBuf, sizeof(payloadBuf));

        ImGui::Image(icon, ImVec2(48, 48));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextUnformatted(node->getName().c_str());
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(COL_TEXT_DIM));
        ImGui::TextUnformatted(node->getPath().c_str());
        ImGui::PopStyleColor();
        ImGui::EndGroup();

        ImGui::EndDragDropSource();
    }
}