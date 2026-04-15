#include "editorassetwindow.h"

#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "core/engine/texture/textureasset.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"

#include "imgui-SFML.h"
#include "glm/glm.hpp"

#include <algorithm>
#include <cstring>

// ─── Colour palette (matches a dark editor theme) ────────────────────────────
static constexpr ImU32 COL_PANEL_BG        = IM_COL32(38,  38,  38,  255);
static constexpr ImU32 COL_TOOLBAR_BG      = IM_COL32(45,  45,  45,  255);
static constexpr ImU32 COL_TILE_HOVER      = IM_COL32(255, 255, 255, 25);
static constexpr ImU32 COL_TILE_SELECTED   = IM_COL32(44,  93,  206, 180);
static constexpr ImU32 COL_TILE_BORDER_SEL = IM_COL32(82, 130, 255, 255);
static constexpr ImU32 COL_ROW_EVEN        = IM_COL32(42,  42,  42,  255);
static constexpr ImU32 COL_ROW_ODD         = IM_COL32(48,  48,  48,  255);
static constexpr ImU32 COL_ROW_HOVER       = IM_COL32(255, 255, 255, 18);
static constexpr ImU32 COL_ROW_SELECTED    = IM_COL32(44,  93,  206, 160);
static constexpr ImU32 COL_TEXT_DIM        = IM_COL32(150, 150, 150, 255);
static constexpr ImU32 COL_SEPARATOR       = IM_COL32(60,  60,  60,  255);
static constexpr ImU32 COL_TREE_SELECTED   = IM_COL32(44,  93,  206, 200);

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

    if (!rootNode) return;

    // Start inside the "assets" folder if it exists, otherwise at root
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

// ─── Top-level render ────────────────────────────────────────────────────────

void MEditorAssetWindow::onGui()
{
    if (!currentDirectoryNode) return;

    // Toolbar spans full width at the top
    drawToolbar();

    // Below toolbar: sources panel | splitter | content area
    float availHeight = ImGui::GetContentRegionAvail().y - 24.0f; // leave space for status bar

    if (showSources)
    {
        ImGui::BeginChild("##sources", ImVec2(sourcesWidth, availHeight), false);
        drawSourcesPanel();
        ImGui::EndChild();

        ImGui::SameLine();

        // Invisible splitter
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
}

// ─── Toolbar ─────────────────────────────────────────────────────────────────

void MEditorAssetWindow::drawToolbar()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(COL_TOOLBAR_BG));
    ImGui::BeginChild("##toolbar", ImVec2(0, 32), false);

    // Back / Forward buttons
    ImGui::SetCursorPosY(6.0f);
    ImGui::BeginDisabled(historyBack.size() <= 1);
    if (ImGui::Button(" < ")) navigateBack();
    ImGui::EndDisabled();

    ImGui::SameLine();

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

    // Sort dropdown
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

    // Show/hide sources panel
    if (ImGui::Button(showSources ? "[ | ]" : "[ ]"))
        showSources = !showSources;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle Sources panel");

    ImGui::SameLine(0, 12);

    // Zoom slider (only meaningful in grid mode)
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##zoom", &zoomLevel, 0.5f, 2.0f, "Zoom %.1f");

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

    // Only show directory nodes in the tree
    if (!node->isDirectory) return;

    bool isCurrentDir = (node == currentDirectoryNode);
    bool hasChildren  = false;
    for (const auto& child : node->getChildrenNodes())
        if (child && child->isDirectory) { hasChildren = true; break; }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth
                             | ImGuiTreeNodeFlags_OpenOnArrow;
    if (!hasChildren)    flags |= ImGuiTreeNodeFlags_Leaf;
    if (isCurrentDir)    flags |= ImGuiTreeNodeFlags_Selected;

    // Highlight selected
    if (isCurrentDir)
    {
        ImVec2 p    = ImGui::GetCursorScreenPos();
        float  w    = ImGui::GetContentRegionAvail().x;
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

void MEditorAssetWindow::drawBreadcrumbs()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(COL_TOOLBAR_BG));
    ImGui::BeginChild("##breadcrumbs", ImVec2(0, 26), false);
    ImGui::SetCursorPosY(5.0f);

    for (int i = 0; i < (int)historyBack.size(); ++i)
    {
        auto* node = historyBack[i];
        ImGui::PushID(i);

        bool isLast = (i == (int)historyBack.size() - 1);
        if (isLast)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,1));
            ImGui::Text("%s", node->getName().c_str());
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(COL_TEXT_DIM));
            if (ImGui::SmallButton(node->getName().c_str()))
            {
                // Trim history back to this point
                historyBack.resize(i + 1);
                currentDirectoryNode = node;
                historyForward.clear();
            }
            ImGui::PopStyleColor();

            ImGui::SameLine(0, 2);
            ImGui::TextDisabled("›");
            ImGui::SameLine(0, 2);
        }
        ImGui::PopID();
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

    // Global context menu (right-click on empty space)
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !pendingContextMenu)
    {
        ImGui::OpenPopup("##ctx_empty");
    }
    if (ImGui::BeginPopup("##ctx_empty"))
    {
        ImGui::TextDisabled("This Folder");
        ImGui::Separator();
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
        {
            if (!caseInsensitiveContains(name, search)) continue;
        }
        if (child->isDirectory) { if (filterDirs)   dirs.push_back(child);  }
        else                    { if (filterFiles)  files.push_back(child); }
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

bool MEditorAssetWindow::matchesSearch(SAssetDirectoryNode* node) const
{
    if (strlen(searchBuffer) == 0) return true;
    return caseInsensitiveContains(node->getName(), searchBuffer);
}

// ─── Grid view ───────────────────────────────────────────────────────────────

void MEditorAssetWindow::drawAssetGrid(SAssetDirectoryNode* root)
{
    if (!root) return;

    MAssetManager* am = MAssetManager::getInstance();
    float iconSize    = 64.0f * zoomLevel;
    float cellSize    = iconSize + 32.0f;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int   colCount   = std::max(1, (int)(panelWidth / cellSize));

    ImGui::Columns(colCount, nullptr, false);

    auto children = getSortedChildren(root, sortMode,
                                      filterDirectories, filterFiles,
                                      searchBuffer);

    for (auto* node : children)
    {
        ImGui::PushID(node);
        drawAssetTile(node, am, iconSize);
        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    // Deselect on click in empty space
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) &&
        !ImGui::IsAnyItemHovered())
    {
        selectedNode = nullptr;
    }
}

void MEditorAssetWindow::drawAssetTile(SAssetDirectoryNode* node,
                                       MAssetManager* am,
                                       float iconSize)
{
    bool isSelected = (node == selectedNode);

    float tileW = iconSize + 20.0f;
    float tileH = iconSize + 48.0f;
    ImVec2 tileSize(tileW, tileH);

    ImGui::BeginGroup();
    ImVec2 p = ImGui::GetCursorScreenPos();

    bool clicked   = ImGui::InvisibleButton("##tile", tileSize);
    bool hovered   = ImGui::IsItemHovered();
    bool dblClick  = hovered && ImGui::IsMouseDoubleClicked(0);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background
    if (isSelected)
    {
        dl->AddRectFilled(p, ImVec2(p.x + tileW, p.y + tileH),
                          COL_TILE_SELECTED, 5.0f);
        dl->AddRect(p, ImVec2(p.x + tileW, p.y + tileH),
                    COL_TILE_BORDER_SEL, 5.0f, 0, 1.5f);
    }
    else if (hovered)
    {
        dl->AddRectFilled(p, ImVec2(p.x + tileW, p.y + tileH),
                          COL_TILE_HOVER, 5.0f);
    }

    if (clicked) selectedNode = node;

    if (dblClick)
    {
        if (node->isDirectory) navigateTo(node);
        else                   onFileDoubleClicked(node);
    }

    // Right-click context menu
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        rightClickedNode = node;
        pendingContextMenu = true;
        ImGui::OpenPopup("##assetctx");
    }
    openContextMenu(node);

    // Icon
    sf::Texture* icon = node->isDirectory
        ? am->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/folder.png")
              ->getTexture()->getCoreTexture()
        : getFileIcon(am, node);

    if (icon)
    {
        // Centre icon inside tile
        ImGui::SetCursorScreenPos(ImVec2(
            p.x + (tileW - iconSize) * 0.5f,
            p.y + 6.0f));
        ImGui::Image(*icon, ImVec2(iconSize, iconSize));
    }

    // Label — centred, wrapped, two lines max
    ImGui::SetCursorScreenPos(ImVec2(p.x + 4.0f, p.y + 10.0f + iconSize));
    drawTruncatedLabel(node->getName(), tileW - 8.0f);

    // Tooltip
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

    // Drag source
    if (hovered && icon)
        doAssetDragSource(MAssetReferenceControl::ASSET_REF_TARGET_KEY.c_str(), *icon, node);

    ImGui::EndGroup();
}

// ─── List view ───────────────────────────────────────────────────────────────

void MEditorAssetWindow::drawAssetList(SAssetDirectoryNode* root)
{
    if (!root) return;

    MAssetManager* am = MAssetManager::getInstance();

    // Header row
    ImGui::Columns(3, "##listcols", false);
    ImGui::SetColumnWidth(0, 300);
    ImGui::SetColumnWidth(1, 120);

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(COL_TEXT_DIM));
    ImGui::Text("  Name");           ImGui::NextColumn();
    ImGui::Text("Type");             ImGui::NextColumn();
    ImGui::Text("Path");             ImGui::NextColumn();
    ImGui::PopStyleColor();

    ImVec2 sep0 = ImGui::GetCursorScreenPos();
    sep0.y -= 2;
    ImGui::GetWindowDrawList()->AddLine(
        sep0, ImVec2(sep0.x + ImGui::GetContentRegionAvail().x, sep0.y),
        COL_SEPARATOR);

    ImGui::Columns(1);

    // Rows
    auto children = getSortedChildren(root, sortMode,
                                      filterDirectories, filterFiles,
                                      searchBuffer);

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
    float rowH      = 22.0f;
    float rowW      = ImGui::GetContentRegionAvail().x;

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Row background
    ImU32 bg = isSelected ? COL_ROW_SELECTED
             : (rowIndex % 2 == 0) ? COL_ROW_EVEN : COL_ROW_ODD;
    dl->AddRectFilled(p, ImVec2(p.x + rowW, p.y + rowH), bg);

    bool clicked  = ImGui::InvisibleButton("##row", ImVec2(rowW, rowH));
    bool hovered  = ImGui::IsItemHovered();
    bool dblClick = hovered && ImGui::IsMouseDoubleClicked(0);

    if (hovered && !isSelected)
        dl->AddRectFilled(p, ImVec2(p.x + rowW, p.y + rowH), COL_ROW_HOVER);

    if (clicked)  selectedNode = node;
    if (dblClick)
    {
        if (node->isDirectory) navigateTo(node);
        else                   onFileDoubleClicked(node);
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        rightClickedNode = node;
        pendingContextMenu = true;
        ImGui::OpenPopup("##assetctx");
    }
    openContextMenu(node);

    // Small icon + name
    sf::Texture* icon = node->isDirectory
        ? am->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/folder.png")
              ->getTexture()->getCoreTexture()
        : getFileIcon(am, node);

    ImGui::SetCursorScreenPos(ImVec2(p.x + 4,  p.y + 3));
    if (icon) ImGui::Image(*icon, ImVec2(16, 16));
    ImGui::SetCursorScreenPos(ImVec2(p.x + 24, p.y + 3));
    ImGui::TextUnformatted(node->getName().c_str());

    // Type column
    ImGui::SetCursorScreenPos(ImVec2(p.x + 304, p.y + 3));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(COL_TEXT_DIM));
    ImGui::TextUnformatted(node->isDirectory ? "Folder"
                                             : FileIO::getFileExtension(node->getPath()).c_str());

    // Path column
    ImGui::SetCursorScreenPos(ImVec2(p.x + 428, p.y + 3));
    ImGui::TextUnformatted(node->getPath().c_str());
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + rowH + 1));

    // Drag
    if (hovered && icon)
        doAssetDragSource(MAssetReferenceControl::ASSET_REF_TARGET_KEY.c_str(), *icon, node);
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
        if (ImGui::MenuItem("Open"))   onFileDoubleClicked(target);
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
    // Use ImGui text wrapping with a two-line cap via clipping
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + maxWidth);
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopTextWrapPos();
}

void MEditorAssetWindow::onFileDoubleClicked(SAssetDirectoryNode* node)
{
    if (!node || node->isDirectory || !node->assetReference) return;
    auto cmd = STR("\"") + node->getAsset()->getFullPath() + STR("\"");
    system(cmd.c_str());
}

sf::Texture* MEditorAssetWindow::getFileIcon(MAssetManager* am, SAssetDirectoryNode* asset) const
{
    // 1. Texture assets show their own preview
    auto* texAsset = dynamic_cast<MTextureAsset*>(asset->getAsset());
    if (texAsset && texAsset->getTexture()->getCoreTexture())
        return texAsset->getTexture()->getCoreTexture();

    // 2. Importer-provided icon
    for (const auto& importer : *MAssetImporter::getImporters())
    {
        auto ext = FileIO::getFileExtension(asset->getPath());
        if (importer->canImport(ext))
        {
            return am->getAsset<MTextureAsset>(importer->getIconPath())
                      ->getTexture()->getCoreTexture();
        }
    }

    // 3. Generic file icon
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
        // BUG FIX: was `sizeof(SString)` which shallow-copies the string object,
        // leaving the receiver with a dangling pointer to freed heap memory.
        // Use a null-terminated char buffer so the payload is plain POD data.
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