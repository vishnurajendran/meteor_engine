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

// ─── Colour palette ────────────────────────────────────────────────────────────
static constexpr ImU32 COL_PANEL_BG          = IM_COL32(30,  30,  30,  255);
static constexpr ImU32 COL_TOOLBAR_BG        = IM_COL32(38,  38,  38,  255);

// Tile card
static constexpr ImU32 COL_TILE_BG           = IM_COL32(36,  36,  36,  255); // base card fill
static constexpr ImU32 COL_TILE_BG_HOVER     = IM_COL32(52,  52,  52,  255); // brightened on hover
static constexpr ImU32 COL_TILE_BORDER       = IM_COL32(60,  60,  60,  255); // idle border
static constexpr ImU32 COL_TILE_BORDER_HOVER = IM_COL32(110, 110, 110, 255); // hover border
static constexpr ImU32 COL_TILE_SELECTED     = IM_COL32(26,  95,  180, 255); // UE blue fill
static constexpr ImU32 COL_TILE_BORDER_SEL   = IM_COL32(70,  150, 255, 255); // UE blue rim
static constexpr ImU32 COL_TILE_SHADOW       = IM_COL32(0,   0,   0,   80);  // drop shadow

// Type bar (bottom strip colour per asset category)
static constexpr ImU32 COL_TYPE_MESH         = IM_COL32(0,  140, 200, 255); // cyan-blue
static constexpr ImU32 COL_TYPE_MATERIAL     = IM_COL32(30, 160,  90, 255); // green
static constexpr ImU32 COL_TYPE_TEXTURE      = IM_COL32(180, 80, 180, 255); // purple
static constexpr ImU32 COL_TYPE_SHADER       = IM_COL32(220,130,  30, 255); // amber
static constexpr ImU32 COL_TYPE_SCENE        = IM_COL32(200, 60,  60, 255); // red
static constexpr ImU32 COL_TYPE_AUDIO        = IM_COL32(60, 180, 180, 255); // teal
static constexpr ImU32 COL_TYPE_FOLDER       = IM_COL32(210,155,  40, 255); // gold
static constexpr ImU32 COL_TYPE_DEFAULT      = IM_COL32(90,  90,  90, 255); // grey

// List rows
static constexpr ImU32 COL_ROW_EVEN          = IM_COL32(36,  36,  36,  255);
static constexpr ImU32 COL_ROW_ODD           = IM_COL32(42,  42,  42,  255);
static constexpr ImU32 COL_ROW_HOVER         = IM_COL32(255, 255, 255, 18);
static constexpr ImU32 COL_ROW_SELECTED      = IM_COL32(26,  95,  180, 200);

// Misc
static constexpr ImU32 COL_TEXT_DIM          = IM_COL32(140, 140, 140, 255);
static constexpr ImU32 COL_SEPARATOR         = IM_COL32(55,  55,  55,  255);
static constexpr ImU32 COL_TREE_SELECTED     = IM_COL32(26,  95,  180, 200);
static constexpr ImU32 COL_RELOAD_FLASH      = IM_COL32(80,  200, 120, 255);

// Tile geometry
// Card is a portrait rectangle: square thumbnail + name area + coloured type bar.
//   cardW = iconSize + 2*THUMB_PAD + 2*CARD_HPAD
//   cardH = cardW    + NAME_AREA_H + TYPE_BAR_H
static constexpr float TILE_ROUNDING  = 6.0f;   // card corner radius
static constexpr float TYPE_BAR_H     = 16.0f;  // coloured bottom strip
static constexpr float NAME_AREA_H    = 30.0f;  // name text zone between thumb and type bar
static constexpr float THUMB_PAD      = 3.0f;   // thumbnail inset inside card
static constexpr float CARD_HPAD      = 5.0f;   // left/right card padding
static constexpr float SHADOW_OFFSET  = 3.0f;   // drop shadow displacement

// ─── Helpers ─────────────────────────────────────────────────────────────────

static bool caseInsensitiveContains(const std::string& haystack,
                                    const std::string& needle)
{
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(),   needle.end(),
        [](char a, char b){ return std::tolower(a) == std::tolower(b); });
    return it != haystack.end();
}

// ─── Constructor / Destructor ────────────────────────────────────────────────

MEditorAssetWindow::MEditorAssetWindow() : MEditorAssetWindow(700, 300) {}

MEditorAssetWindow::MEditorAssetWindow(int x, int y) : MImGuiSubWindow(x, y)
{
    title = "Asset Browser";

    auto* editorAssetManager = dynamic_cast<MEditorAssetManager*>(MAssetManager::getInstance());
    rootNode = editorAssetManager ? editorAssetManager->getAssetRootNode() : nullptr;

    // Seed the reload counter so we don't flash on startup.
    if (editorAssetManager)
        lastSeenReloadCount = editorAssetManager->getTotalHotReloadCount();

    if (!rootNode) return;

    // Start inside the "assets" folder if it exists, otherwise at root.
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

// ─── Hot reload sync ──────────────────────────────────────────────────────────

void MEditorAssetWindow::tickAndSync(float deltaTime)
{
    auto* editorAM = dynamic_cast<MEditorAssetManager*>(MAssetManager::getInstance());
    if (!editorAM) return;

    // ── Tree staleness check ──────────────────────────────────────────────────
    // refresh() deletes the entire tree and rebuilds it. Our cached node
    // pointers become dangling the moment that happens. Detect it by comparing
    // the root pointer — the manager always returns its current live root.
    if (rootNode != editorAM->getAssetRootNode())
        syncWithAssetManager();

    // ── Hot-reload flash ──────────────────────────────────────────────────────
    // tickHotReload() runs in MEditorApplication::run(), before onGui() is
    // called, so by the time we get here the counter is already updated.
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
    auto* editorAM = dynamic_cast<MEditorAssetManager*>(MAssetManager::getInstance());
    if (!editorAM) return;

    // Snapshot everything as path strings BEFORE touching any pointers.
    const SString savedCurrentPath  = currentDirectoryNode
                                        ? currentDirectoryNode->getPath() : SString();
    const SString savedSelectedPath = selectedNode
                                        ? selectedNode->getPath() : SString();

    std::vector<SString> savedHistoryBack, savedHistoryForward;
    for (auto* n : historyBack)    savedHistoryBack.push_back(n ? n->getPath() : SString());
    for (auto* n : historyForward) savedHistoryForward.push_back(n ? n->getPath() : SString());

    // Drop all stale pointers.
    rootNode             = nullptr;
    currentDirectoryNode = nullptr;
    selectedNode         = nullptr;
    rightClickedNode     = nullptr;
    historyBack.clear();
    historyForward.clear();

    rootNode = editorAM->getAssetRootNode();
    if (!rootNode) return;

    // Re-map current directory directly — do NOT call navigateTo() since that
    // would push to the history stack and clear the forward stack.
    // Fall back to root only if the folder itself was deleted.
    currentDirectoryNode = findNodeByPath(rootNode, savedCurrentPath);
    if (!currentDirectoryNode)
        currentDirectoryNode = rootNode;

    // Re-map both history stacks entry by entry, dropping any entry that no
    // longer exists or isn't a directory (files should never be in history).
    for (const auto& path : savedHistoryBack)
    {
        if (path.empty()) continue;
        if (auto* node = findNodeByPath(rootNode, path))
            if (node->isDirectory)
                historyBack.push_back(node);
    }
    for (const auto& path : savedHistoryForward)
    {
        if (path.empty()) continue;
        if (auto* node = findNodeByPath(rootNode, path))
            if (node->isDirectory)
                historyForward.push_back(node);
    }

    // Re-map selection — cleared only if the asset was deleted.
    if (!savedSelectedPath.empty())
        selectedNode = findNodeByPath(rootNode, savedSelectedPath);
}

// ─── processPendingPing ──────────────────────────────────────────────────────

void MEditorAssetWindow::processPendingPing()
{
    auto* editorAM = dynamic_cast<MEditorAssetManager*>(MAssetManager::getInstance());
    if (!editorAM || !rootNode) return;

    const SString assetId = editorAM->consumePendingPing();
    if (assetId.empty()) return;

    SAssetDirectoryNode* parent = nullptr;
    SAssetDirectoryNode* target = findNodeByAssetId(rootNode, assetId, &parent);
    if (!target) return;

    // Navigate to the parent folder so the asset is visible.
    if (parent && parent != currentDirectoryNode)
        navigateTo(parent);

    // Highlight in browser and route to inspector via handle.
    selectedNode = target;
    selectAssetForInspector(target);
}

// BFS: find the asset node whose assetReference has the given assetId.
// outParent is set to the immediate parent directory node if provided.
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

// ─── findNodeByPath ───────────────────────────────────────────────────────────

// BFS over the tree — paths are unique so we stop at the first match.
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

// ─── Top-level render ────────────────────────────────────────────────────────

void MEditorAssetWindow::onGui(float deltaTime)
{
    // Must be first — keeps node pointers valid and arms the flash timer.
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

    // ── Status bar ────────────────────────────────────────────────────────────
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

    // Hot-reload indicator — fades out over RELOAD_FLASH_DURATION seconds.
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

// ─── Toolbar ─────────────────────────────────────────────────────────────────

void MEditorAssetWindow::drawToolbar()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(COL_TOOLBAR_BG));
    ImGui::BeginChild("##toolbar", ImVec2(0, 32), false);

    ImGui::SetCursorPosY(6.0f);

    // Back
    ImGui::BeginDisabled(historyBack.size() <= 1);
    if (ImGui::Button(" < ")) navigateBack();
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Forward
    ImGui::BeginDisabled(historyForward.empty());
    if (ImGui::Button(" > "))
    {
        auto* next = historyForward.back();
        historyForward.pop_back();
        historyBack.push_back(currentDirectoryNode);
        currentDirectoryNode = next;
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0, 12);

    // Search
    ImGui::SetNextItemWidth(220);
    ImGui::InputTextWithHint("##search", "  Search assets...", searchBuffer, sizeof(searchBuffer));

    ImGui::SameLine(0, 12);

    // Sort
    ImGui::SetNextItemWidth(110);
    const char* sortLabels[] = { "Name", "Type", "Date" };
    int sortIdx = (int)sortMode;
    if (ImGui::Combo("##sort", &sortIdx, sortLabels, 3))
        sortMode = (EAssetSortMode)sortIdx;

    ImGui::SameLine(0, 12);

    // View mode toggle
    if (ImGui::Button(viewMode == EAssetViewMode::Grid ? "[#]" : "[=]"))
        viewMode = (viewMode == EAssetViewMode::Grid) ? EAssetViewMode::List
                                                       : EAssetViewMode::Grid;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle Grid / List view");

    ImGui::SameLine(0, 6);

    // Sources panel toggle
    if (ImGui::Button(showSources ? "[ | ]" : "[ ]"))
        showSources = !showSources;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle Sources panel");

    ImGui::SameLine(0, 12);

    // Zoom
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##zoom", &zoomLevel, 0.5f, 2.0f, "Zoom %.1f");

    // ── Reload flash dot ──────────────────────────────────────────────────────
    // A small coloured indicator in the right margin that fades after a reload.
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

// ─── Left panel – folder tree ────────────────────────────────────────────────

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

// ─── Breadcrumbs ─────────────────────────────────────────────────────────────
// Derived each frame from currentDirectoryNode->getPath(), NOT from historyBack.
// historyBack is navigation history (for the < > buttons); using it for display
// causes file names, duplicates, and stale entries to appear in the bar.

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

    // ── Root crumb ────────────────────────────────────────────────────────────
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
        {
            historyBack.push_back(currentDirectoryNode);
            historyForward.clear();
            currentDirectoryNode = rootNode;
            selectedNode = nullptr;
        }
        ImGui::PopStyleColor();

        // ── Segment crumbs ────────────────────────────────────────────────────
        // Split "assets/textures/hdr" into ["assets", "textures", "hdr"] and
        // walk the tree, making each intermediate segment a clickable button.
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
                {
                    historyBack.push_back(currentDirectoryNode);
                    historyForward.clear();
                    currentDirectoryNode = walker;
                    selectedNode = nullptr;
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopID();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ─── Content area dispatcher ─────────────────────────────────────────────────

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
        // ── Create submenu — mirrors "Assets/Create" menubar entries ─────────────
    // getNodeAtPath walks the registered tree, so any new MMenubarItem under
    // "Assets/Create" automatically appears here without further changes.
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

// ─── Helpers: get sorted + filtered children ─────────────────────────────────

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

// ── Per-asset-type colour strip ───────────────────────────────────────────────
// Returns the accent colour for the bottom type bar based on file extension or
// asset category, mirroring Unreal's content browser colour coding.
static ImU32 getTileTypeColor(SAssetDirectoryNode* node)
{
    if (!node) return COL_TYPE_DEFAULT;
    if (node->isDirectory) return COL_TYPE_FOLDER;

    const std::string path = node->getPath().str();
    auto ext = path.substr(path.find_last_of('.') + 1);
    for (auto& c : ext) c = (char)std::tolower(c);

    // Mesh formats
    if (ext == "obj" || ext == "fbx" || ext == "gltf" || ext == "glb" ||
        ext == "dae" || ext == "3ds" || ext == "stl")
        return COL_TYPE_MESH;

    // Material
    if (ext == "mat" || ext == "material")
        return COL_TYPE_MATERIAL;

    // Texture / image
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
        ext == "tga" || ext == "hdr" || ext == "exr" || ext == "dds")
        return COL_TYPE_TEXTURE;

    // Shader / cubemap
    if (ext == "mesl" || ext == "glsl" || ext == "vert" || ext == "frag" ||
        ext == "cubemap")
        return COL_TYPE_SHADER;

    // Scene
    if (ext == "mscene" || ext == "scene")
        return COL_TYPE_SCENE;

    // Audio
    if (ext == "wav" || ext == "ogg" || ext == "mp3" || ext == "flac")
        return COL_TYPE_AUDIO;

    return COL_TYPE_DEFAULT;
}

// ── Short type label ──────────────────────────────────────────────────────────
// Returns std::string (not const char*) to avoid returning a pointer into a
// local that has gone out of scope — previously caused garbage like "P?".
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
    if (ext == "cubemap")                               return "Cubemap";
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

// ─── Grid view ───────────────────────────────────────────────────────────────

void MEditorAssetWindow::drawAssetGrid(SAssetDirectoryNode* root)
{
    if (!root) return;

    MAssetManager* am  = MAssetManager::getInstance();
    float iconSize     = 64.0f * zoomLevel;
    // Card width = iconSize + 2*CARD_HPAD, plus a small column gap.
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
        selectedNode = nullptr;
}

void MEditorAssetWindow::drawAssetTile(SAssetDirectoryNode* node,
                                       MAssetManager* am,
                                       float iconSize)
{
    const bool isSelected = (node == selectedNode);

    // ── Card geometry ─────────────────────────────────────────────────────────
    // Portrait rectangle:  [thumbnail — square] / [name label] / [type bar]
    const float thumbSz = iconSize;                          // thumbnail is square
    const float cardW   = thumbSz + CARD_HPAD * 2.0f;       // card width
    const float cardH   = thumbSz + NAME_AREA_H + TYPE_BAR_H + THUMB_PAD * 2.0f;

    // ── Resolve icon / thumbnail ──────────────────────────────────────────────
    sf::Texture* icon        = nullptr;
    bool         isThumbnail = false;

    if (node->isDirectory)
    {
        icon = am->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/folder.png")
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

    // ── Hit-test (drag source attached here) ─────────────────────────────────
    bool clicked  = ImGui::InvisibleButton("##tile", ImVec2(cardW, cardH));
    bool hovered  = ImGui::IsItemHovered();
    bool dblClick = hovered && ImGui::IsMouseDoubleClicked(0);

    if (!node->isDirectory && icon)
        doAssetDragSource(MAssetReferenceControl::ASSET_REF_TARGET_KEY.c_str(), *icon, node);

    if (clicked)  { selectedNode = node; selectAssetForInspector(node); }
    if (dblClick) { if (node->isDirectory) navigateTo(node); else onFileDoubleClicked(node); }
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        rightClickedNode   = node;
        pendingContextMenu = true;
        ImGui::OpenPopup("##assetctx");
    }
    openContextMenu(node);

    // ── Draw ──────────────────────────────────────────────────────────────────
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const ImVec2 br       (p.x + cardW,         p.y + cardH);
    const ImVec2 thumbTL  (p.x + CARD_HPAD,     p.y + THUMB_PAD);
    const ImVec2 thumbBR  (p.x + CARD_HPAD + thumbSz, p.y + THUMB_PAD + thumbSz);
    const ImVec2 nameTL   (p.x,                 thumbBR.y);
    const ImVec2 nameBR   (br.x,                nameTL.y + NAME_AREA_H);
    const ImVec2 typeBarTL(p.x,                 nameBR.y);
    const ImU32  typeCol  = getTileTypeColor(node);

    // Drop shadow
    dl->AddRectFilled(
        ImVec2(p.x + SHADOW_OFFSET, p.y + SHADOW_OFFSET),
        ImVec2(br.x + SHADOW_OFFSET, br.y + SHADOW_OFFSET),
        COL_TILE_SHADOW, TILE_ROUNDING);

    // Card background
    const ImU32 bgCol = isSelected ? COL_TILE_SELECTED
                      : hovered    ? COL_TILE_BG_HOVER
                                   : COL_TILE_BG;
    dl->AddRectFilled(p, br, bgCol, TILE_ROUNDING);

    // ── Thumbnail / icon area ─────────────────────────────────────────────────
    if (icon)
    {
        dl->PushClipRect(thumbTL, thumbBR, true);

        if (isThumbnail)
        {
            // Thumbnail fills its square zone completely.
            dl->AddImage(
                (ImTextureID)(intptr_t)icon->getNativeHandle(),
                thumbTL, thumbBR,
                ImVec2(0, 0), ImVec2(1, 1),
                IM_COL32(255, 255, 255, 255));
        }
        else
        {
            // Generic icon: dark inset bg, icon centred at 60% size.
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

    // ── Name label area ───────────────────────────────────────────────────────
    // Background matches the card so it blends in; text is centred vertically.
    // Name is truncated with "…" if it overflows the card width.
    {
        const float  maxNameW  = cardW - CARD_HPAD * 2.0f;
        const float  lineH     = ImGui::GetTextLineHeight();
        const float  textY     = nameTL.y + (NAME_AREA_H - lineH) * 0.5f;

        // Truncate name with ellipsis if needed.
        const std::string fullName = node->getName();
        const char*       nameC    = fullName.c_str();
        std::string       truncated;

        if (ImGui::CalcTextSize(nameC).x > maxNameW)
        {
            // Binary-search the longest prefix that fits with "…" appended.
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

        // Centre the text horizontally.
        const float textW = ImGui::CalcTextSize(nameC).x;
        const float textX = p.x + CARD_HPAD + (maxNameW - textW) * 0.5f;

        dl->AddText(ImVec2(textX, textY),
                    isSelected ? IM_COL32(255, 255, 255, 255)
                               : IM_COL32(210, 210, 210, 255),
                    nameC);
    }

    // ── Type bar ─────────────────────────────────────────────────────────────
    // Rounded only on the bottom corners so it merges with the card edge.
    dl->AddRectFilled(typeBarTL, br, typeCol,
                      TILE_ROUNDING, ImDrawFlags_RoundCornersBottom);

    // Type label — truncate the same way if needed.
    {
        std::string  labelStr  = getTileTypeLabel(node);  // owns the string
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

    // ── Border ────────────────────────────────────────────────────────────────
    const ImU32  borderCol = isSelected ? COL_TILE_BORDER_SEL
                           : hovered    ? COL_TILE_BORDER_HOVER
                                        : COL_TILE_BORDER;
    const float  borderW   = isSelected ? 2.0f : 1.0f;
    dl->AddRect(p, br, borderCol, TILE_ROUNDING, 0, borderW);

    // ── Tooltip ───────────────────────────────────────────────────────────────
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


// ─── List view ───────────────────────────────────────────────────────────────

void MEditorAssetWindow::drawAssetList(SAssetDirectoryNode* root)
{
    if (!root) return;

    MAssetManager* am = MAssetManager::getInstance();

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
        selectedNode = nullptr;
}

void MEditorAssetWindow::drawAssetListRow(SAssetDirectoryNode* node,
                                          MAssetManager* am,
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

    // ── Resolve icon BEFORE InvisibleButton so the drag source can use it. ────
    sf::Texture* icon = nullptr;
    if (node->isDirectory)
    {
        icon = am->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/folder.png")
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

    // Drag source attached directly to the InvisibleButton.
    if (!node->isDirectory && icon)
        doAssetDragSource(MAssetReferenceControl::ASSET_REF_TARGET_KEY.c_str(), *icon, node);

    if (hovered && !isSelected)
        dl->AddRectFilled(p, ImVec2(p.x + rowW, p.y + rowH), COL_ROW_HOVER);

    if (clicked)  { selectedNode = node; selectAssetForInspector(node); }
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

// ─── Context menu ─────────────────────────────────────────────────────────────

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

    ImGui::Separator();

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


// ─── Navigation ──────────────────────────────────────────────────────────────

void MEditorAssetWindow::navigateTo(SAssetDirectoryNode* node)
{
    if (!node || node == currentDirectoryNode) return;

    historyForward.clear();
    if (currentDirectoryNode)
        historyBack.push_back(currentDirectoryNode);

    currentDirectoryNode = node;
    selectedNode         = nullptr;
}

void MEditorAssetWindow::navigateBack()
{
    if (historyBack.size() <= 1) return;

    historyForward.push_back(currentDirectoryNode);
    currentDirectoryNode = historyBack.back();
    historyBack.pop_back();
    selectedNode = nullptr;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

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
    auto* editorAssetManager = dynamic_cast<MEditorAssetManager*>(MAssetManager::getInstance());
    if (editorAssetManager)
        editorAssetManager->openAsset(node->assetReference);
}



sf::Texture* MEditorAssetWindow::getFileIcon(MAssetManager* am, SAssetDirectoryNode* asset) const
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
               "meteor_assets/engine_assets/icons/file-default.png")
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